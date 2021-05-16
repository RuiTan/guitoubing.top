---
title: Tensorflow多GPU并行计算
date: 2020-10-02 23:33:40
tags:
    - TensorFlow
    - 图像分割
categories:
    - 深度学习
---

> 笔者本科毕设做的是遥感图像分割相关的研究，当时采用的Potsdam数据集由于所有图像GSD(对地观测距离)固定且适中，因此将训练时的batch_size设置为了16，基本能满足每个批次中都包含所有地表要素的需求。然而[NAIC2020](https://naic.pcl.ac.cn/)数据集中包含的数据的GSD从0.1m-4m不等，0.1m的GSD意味着一张256x256的影像切片中甚至能够看到一根树干的轮廓，然而这也使得该切片包含的要素种类很少！以致于一个batch中的16张切片可能也无法包含所有的要素。
> 一旦batch训练数据无法包含所有要素，就使得当次训练结果对于未被包含的要素基本不具备识别能力，表现为第n次训练和第n+1次训练结果梯度变化很大，有点像拆了东墙补西墙；而限制了batch_size的最根本原因是GPU限制，本模型完全跑起来需要约9G显存(输入大小为[16,256,256,3]，即16张256x256大小的RGB图像)，服务器上的GPU仅有11G，虽然显存不大，但是GPU多呀，4块2080Ti现在只用了一块。
> 因此笔者考虑使用多GPU来增大batch_size，以此来使得每个batch训练的参数结果对所有要素都较为适配。

<!--more-->

## 并行计算

深度学习方法采用并行计算的方法一般包括两种：`数据并行`和`模型并行`。

### 数据并行

考虑一个事实：深度学习方法在使用随机梯度下降法进行训练时，每次训练我们会尝试随机取出`n`个数据点去计算当前这个批次的梯度，然而当前这个批次计算出的梯度相对于整个数据集的真实梯度来说很不准确，同时GPU显存通常不足以容下我们所有的训练数据，因此随机梯度下降法通常需要`①增大每个批次的数据点数量`、`②使用更多的训练轮次`来更快、更准地进行模型的收敛。

NAIC2020数据集中包含了数十万张切片，由于multi-scale特性的存在，每个切片可能包含1-8个要素不等。由于先前训练时的batch_size为16，很难在每个batch里得到包含所有8个要素的16张切片，而由于GPU显存的限制，16已经达到了极限，因此考虑使用多GPU来解决batch_size这一瓶颈。

数据并行方法在操作时，允许输入模型的批次数量更大，而后将大批次分割为很多`小批次`，而后在每个GPU上计算一个小批次，每个GPU的梯度估计结果进行汇总后，进行`加权平均`，最终`求和`就得到了大批次的梯度估计结果。

> [证明过程](https://blog.csdn.net/LoseInVain/article/details/105808818)：
> ![数据并行有效性证明过程](http://imgbed.guitoubing.top/20201003001646.png)
> 
> 当每个小批次是由大批次平均分配时，那么：
> m1=m2=···=mk=n/k

下图解释了数据并行的[流程](https://www.zhihu.com/question/53851014)：
![数据并行模型](http://imgbed.guitoubing.top/20201003001716.png)

每个节点输入不同的`Data Shards`，即一个小批次，通过节点上的模型训练得到各自的梯度，而节点中的模型参数是相同的，这样的一个节点通常称作`worker`；而后每个`worker`会把各自计算得到的梯度送到`ps server`进行汇总操作（加权平均）并传回到各个节点进行模型的更新。

### 模型并行

待更新

## 实际操作

笔者代码使用的是`TensorFlow-GPU 2.0`+`Keras 2.3`，采用数据并行的方式增大`batch_size`；训练时，首先需要进行如下设置：

```python
os.environ["CUDA_VISIBLE_DEVICES"] = '0,1,2,3'
```

这句话是让4块GPU都对程序**可见**，然而可见并不是指**成功调用**了。

当设置`os.environ["CUDA_VISIBLE_DEVICES"] = '0,1,2,3'`变量时，程序仍然可以正常训练，但是没有达到预期的多GPU效果，各GPU占用如下：

|        | GPU0 | GPU1 | GPU2 | GPU3 |
| :------------- | :----------: | :-----------: | :----------: | :----------: |
| 显存占用(MB) | 10870 | 155 | 155 | 155 |
| GPU占用(%) | 87 | 0 | 0 | 0 |

可以看到，还是GPU1-3虽然有显存占用，但是GPU基本无占用，说明这三个GPU仅加载了模型，并没有执行实际的计算任务；因此当增大batch_size时，程序并没有自动唤醒GPU1-3来帮助分担计算任务，而是报出`内存不足`的错误，即使GPU1-3均有空余容量。

在TensorFlow里，需要自行指定操作对应的`device`即GPU：

```python
# image:[64,256,256,3], label:[64,256,256]
# small_batch_size = batch_size/N_GPUs = 64/4 = 16
with tf.variable_scope(tf.get_variable_scope()):
    for i in tqdm(range(args.N_GPUs)):
        with tf.device('/gpu:%d' % i):
            start = i * small_batch_size
            end = (i + 1) * small_batch_size
            x = image[start:end, :, :, :]
            y = label[start:end, :, :]
            logits = model.forward(x)
            ……
            # calc loss with logits
            ……
            grads = optimizer.compute_gradients(loss=loss, var_list=tf.trainable_variables())
            all_grads.append(grads)
# merge gradients
grads = average_gradients(all_grads)
```

上述代码中，我将batch_size设置为64，并将每个batch平均分配给4个GPU来进行计算任务，每个GPU需要前向传递更新自己的参数。一个训练伦次中，当4个GPU都计算完了梯度后，会传给主GPU(也可以是CPU，这里我使用GPU0为主GPU)进行梯度的合并，而后反向传递回并更新4个GPU的模型参数。训练过程中，GPU占用如下：

|        | GPU0 | GPU1 | GPU2 | GPU3 |
| :------------- | :----------: | :-----------: | :----------: | :----------: |
| 显存占用(MB) | 8793 | 8787 | 8787 | 8787 |
| GPU占用(%) | 30-55 | 30-50 | 30-50 | 30-50 |

可见，由于GPU0是主GPU，需要完成梯度的合并工作，因此需要占用稍微多一点的内存和GPU。

当增加了GPU数量以进行并行计算时，通常会由于GPU之间的通信、梯度汇总等原因导致批次的训练速度变慢：本机4个GPU并行计算时，训练速度约为`1.2次/s`，单GPU训练速度约为`2次/s`；然而由于4GPU每轮次输入了64个切片，是单GPU的4倍之多，因此4GPU1.25w次训练就能达到单GPU5w次的训练效果，总体训练时间还是缩短了。