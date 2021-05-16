---
title: Java - 深入垃圾回收
date: 2019-04-11 10:36:03
tags:
    - 垃圾回收
    - 可达性分析
    - HotSpot
categories:
    - Java
---

## 可达性分析算法

算法的基本思路是通过一系列称为`GC Roots`的对象作为起始点，从这些节点向下搜索，搜索所走过的路径称作`引用链`。

<!-- more -->

![image-20190408154950595](/images/可达性分析算法.png)

可作为GC Roots的对象包括下面几种：

- **虚拟机栈**(栈帧中的**本地变量表**)中引用的对象
- 方法区中**类静态属性**引用的对象
- 方法去中**常量**引用的对象
- 本地方法栈中`JNI`引用的对象

## 引用

### 强引用

程序代码中普遍存在的，类似于`Object obj = new Object();`这类的引用，**只要强引用还存在，GC永远不会回收掉被引用的对象**。

### 软引用

用来描述一些还**有用但并非必需**的对象。对于软引用关联着的对象，在系统将要发生内存溢出异常之前，将会把这些对象列进回收范围之中进行第二次回收。可用SoftReference类来实现。

### 弱引用

比软引用强度低，被弱引用关联的对象**只能生存到下一次垃圾收集发生之前**。可用WeakReference类来实现。

### 虚引用

最弱的引用。一个对象是否有虚引用的存在，完全不会对其生存时间构成影响，也无法通过虚引用来取得一个对象实例。

## 再谈finalize()

即使在上述可达性分析算法中不可达的对象，**也并非是非死不可的**。

### 过程

要真正宣告一个对象死亡，至少要经历两次标记过程：

1. 如果对象在进行可达性分析后没有与`GC Roots`相连接的引用链
2. 进行筛选，条件是对象是否有必要执行`finalize()`方法。当对象没有覆盖`finalize()`方法，或者`finalize()`方法以及被调用过，则虚拟机认定`没有必要执行`，此时才宣判对象已死

### 再生

当有必要执行`finalize()`方法时，则对象就会有拯救自己的机会，如下：

```java
import java.util.concurrent.TimeUnit;

public class FinalizeEscapeGC {
    public static FinalizeEscapeGC SAVE_HOOK = null;

    public void isAlive() {
        System.out.println("yes, i am still alive.");
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        System.out.println("finalize method executed!");
        FinalizeEscapeGC.SAVE_HOOK = this;
    }

    public static void main(String[] args) throws InterruptedException {
        SAVE_HOOK = new FinalizeEscapeGC();

        // 对象第一次成功拯救自己
        SAVE_HOOK = null;
        System.gc();
        // 因为finalize方法优先级很低，所以暂停0.5s以等待它
        TimeUnit.MILLISECONDS.sleep(500);
        if (SAVE_HOOK != null){
            SAVE_HOOK.isAlive();
        } else {
            System.out.println("no, i am dead!");
        }

        // 下面的代码与上面完全相同，但是此次自救却失败了
        SAVE_HOOK = null;
        System.gc();
        TimeUnit.MILLISECONDS.sleep(500);
        if (SAVE_HOOK != null){
            SAVE_HOOK.isAlive();
        } else {
            System.out.println("no, i am dead!");
        }
    }
}
/**Output:
finalize method executed!
yes, i am still alive.
no, i am dead!
**/.
```

从上述代码及其结果可看到，`SAVE_HOOK`对象的`finalize()`方法确实被GC触发过，其本身也在垃圾收集之前成功逃脱了。但是注意，由于一个对象的finalize()只会被执行一遍，因此上述代码中第二次将逃脱失败，无法完成自救。

## HotSpot的算法实现

> 关于GC的几种主流实现方法（简单记忆）：
>
> - 保守式GC(Conservative GC)：指JVM不记录内存上的某个数据应该被解读为引用类型还是其他类型。
> - 半保守式GC(Conservative with respect to the roots)：让对象带有足够的元数据
> - 准确式GC(Exact GC)：提供特定数据结构保存对象引用

### 枚举根节点

枚举根节点这一过程是**必须要停顿所有Java执行线程**，即`Stop The World`。因为要保证这段时间的引用不变性。

`Java`中使用`OopMap`来存储对象引用，以实现`准确性GC`，同时也避免了垃圾回收时需要遍历栈的每个位置。

### 安全点 Safepoint

`Hotspot`虚拟机**只在到达`Safepoint`位置暂停**，以进行GC。

程序中**指令序列复用**的指令，例如方法调用、循环跳转、异常跳转等情况，才会产生`Safepoint`。

在多线程中，有两种中断方案可供选择：

- **抢先式中断**：GC发生时，将所有线程中断，而后让不在安全点上的线程恢复，直到跑到安全点。
- **主动式中断**：设置一个标志，各个线程主动轮询这个标志，发现中断标志为真时就自己中断挂起。

### 安全区域 Safe Region

安全区域是为了解决**程序`不执行`的时候，程序无法进入安全点的情况**，例如线程处于`Sleep`或者`Blocked`状态时。

安全区域指的是一段代码片段之中，引用关系不会发生变化，因此在这个区域的任何地方开始GC都是安全的。

当线程执行到安全区域的代码中时：

- 首先标识自己已经进入安全区域，此时JVM发起GC时就**无需**询问处于安全区域状态的线程了，直接回收
- 在线程要离开安全区域时，需要**检查JVM是否已经完成了根节点枚举**，如果完成了则线程继续执行，否则**必须要等待直到收到可以安全离开安全区域的信号为止**。

## 垃圾收集器

### Serial收集器

`Serial收集器`是一个**单线程的收集器**，它**只会使用一个CPU或一条收集线程去完成垃圾收集工作**，同时它进行垃圾收集时，**必须暂停其他所有工作线程**。

### ParNew收集器

`Serial收集器`的**多线程**版本。

目前只有`Serial`和`ParNew`能够与`CMS收集器`配合工作。

### Parallel Scavenge收集器

此收集器的侧重点放在`吞吐量`上，吞吐量就是CPU用于运行用户代码与CPU总消耗时间的比值，即`吞吐量=运行用户代码时间/(运行用户代码时间+垃圾收集时间)`。

注意，**吞吐量与垃圾收集速度无太大关系**。

同时采用此类收集器的虚拟机可根据系统运行状况手机性能监控信息，**动态调整参数**以提供最合适的停顿时间或最大的吞吐量。

### Serial Old收集器

`Serial收集器`的老年代版本，两大用途：

1. 与`Parallel Scavenge收集器`搭配使用
2. 作为`CMS收集器`的后备预案，在并发收集发生`Concurrent Mode Failure`时使用。

### Parallel Old收集器

`Parallel Scavenge收集器`的老年代版本。

### CMS收集器

CMS收集器是一种**以获取最短回收停顿时间为目标**的收集器。

#### 收集过程

CMS收集器收集过程分为4个步骤：

- **初始标记：**需要`Stop The World`，标记GC Roots能直接关联到的对象，速度很快。
- **并发标记：**不需要`Stop The World`，进行`GC Roots Tracing`。
- **重新标记：**需要`Stop The World`，标记因用户程序继续运作而导致变动的那一部分对象的标记记录。
- **并发清除：**不需要`Stop The World`，进行清除。

#### 缺点

- 对CPU资源很敏感，当CPU资源紧张时，用户程序速度下降很明显。
- 无法处理浮动垃圾，即在标记之后出现的垃圾，只能留到下一次GC时再清理掉。同时使用CMS时，由于需要预留空间给用户线程，因此不能等到老年代几乎全部被填满了再进行收集。此时当CMS预留的内存无法满足程序需要，就会出现一次`Concurrent Mode Failure`失败，这是就使用后备收集器`Serial Old`。
- 标记-清除算法会产生空间碎片。

### G1收集器(Garbage First)

#### 特点

- 并行和并发
- 分代收集
- 空间整合
- 可预测的停顿

G1收集器中新生代和老年代不再是物理隔离的，它**将整个Java堆划分为多个大小相等的独立区域(`Region`)**。

由于`Region`之间可能存在相互引用的关系，所以**使用`Remembered Set`来记录从`其他Region`引用`当前Region`的引用信息**，**`Remembered Set`是一种抽象概念，`Card Table`是其一种实现方式**。

实际上，G1相关算法是个很复杂的过程，见[R大的帖子](<https://hllvm-group.iteye.com/group/topic/44381#post-272188>)，需要进一步研究。

#### 收集过程

- 初始标记
- 并发标记
- 最终标记
- 筛选回收

## 内存分配与回收策略

### 对象优先在Eden分配

大多数情况下，对象在新生代Eden区中分配，当Eden区没有足够空间进行分配时，虚拟机发起一次MinorGC。

### 大对象直接进入老年代

大对象指的是**需要大量连续内存空间的Java对象**，例如很长的字符串以及数组。更糟糕的是产生一群**朝生夕灭**的**短命大对象**。

### 长期存活的对象将进入老年代

虚拟机给每个对象定义了一个`对象年龄计数器`，当年龄增长到阈值时，就可以晋升到老年代。阈值默认为15，也可通过`MaxTenuringThredhold`参数设置。

### 动态对象年龄判断

当`Survivor`空间中**相同年龄所有对象大小总和**大于`Survivor`空间的一半，年龄大于等于该年龄的对象就直接进入老年代。