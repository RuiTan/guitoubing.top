---
title: Hadoop-Tags
date: 2018-12-12 00:33:02
tags:
    - Hadoop
    - 云计算
categories:
    -- archive
---

> 更新中……

<!-- more -->

# Hadoop Tags

## HDFS

1. NameNode内存要求较高，存储文件系统元结构（文件目录结构、分块情况、每块位置、权限等）

2. 文件分块默认最小块128M

3. `jps`命令查看NameNode/DataNode是否启动

   1. ~~jps在jdk8u191中好像不适用，暂未找到解决方法~~

4. `ip:9870`利用web界面查看Hadoop节点信息（Mac上端口号为`50070`）

5. 进入用户目录下的`.ssh`目录，执行`ssh-keygen -t rsa`创建公钥私钥，使用`ssh-copy-id ${hostname}`将公钥传给每个节点（NameNode和DataNode都需要）

6. 使用`hadoop fs -ls /`查看Hadoop上所有文件，使用`hadoop fs -put ${filename} /`上传文件…

7. `hdfs-site.xml`中修改`dfs.replication`配置可修改文件备份份数（默认为3），修改`dfs.namenode.heartbead.recheck-interval`指定Hadoop检查机器运行情况的时间间隔（默认3000000ms）

   注意：

   1. 例如当备份份数为2时，现在有三台DataNode机器，文件被分为2个block，block1位于1和2上，block2位于1和3上，这是若机器3宕机了，hdfs会在设定的`dfs.namenode.heartbead.recheck-interval`时间间隔内检查出机器3，此时block2数量变为1，hdfs会自动将1中的block2复制一份到另外一台可用机器上（此处为2）。当机器3恢复运行时，3中备份的block2会自动删除。
   2. 当使用java访问hdfs时，不会使用`hdfs-site.xml`中的`dfs.replication`，而会默认使用3，可在java的`configuration`中配置为指定值

8. `分鱼展`:分块、冗余、可扩展

## Yarn

1. ResourceManage

2. NodeManage一般与DataNode放一起 

3. Yarn逻辑上与HDFS完全分离，但一般绑定HDFS一起使用

4. `yarn-site.xml`的配置

   **注意：master与slaves都需要进行配置。**

   ```xml
    <property>
        <name>yarn.resourcemanager.hostname</name>
        <value>master</value>
    </property>
   ```

5. `mapred-site.xml`的配置

   **注意：**

   - 仅NameNode需要配置
   - MapReduce不一定需要Yarn
   - 若不配MapReduce，其会仅在单机跑

   ```xml
   <property>
       <name>mapreduce.framework.name</name>
       <value>yarn</value>
   </property>
   ```

# TODO List

- 安全与权限（kerberos）
- Secondary NameNode（check point NameNode）
- HA（High Ability）实现
- Federation，超大规模数据中心