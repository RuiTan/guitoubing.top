---
title: Hadoop-Tags
date: 2018-12-12 00:33:02
tags:
    - Hadoop
categories:
    - Hadoop
---


# Hadoop Tags

## HDFS

<!-- more -->

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

# Hive

1. 创建Hive、Hadoop环境变量，方便敲命令

2. 修改`hive-site.xml`

   1. hive的conf目录下刚初始化时没有hive-site.xml，需要将`hive-default.xml.template`复制一份更名为hive-site.xml
   2. 将hive-site.xml中所有的(4个)`${system:java.io.tmpdir}`替换为`/usr/local/hive/tmp`，将所有的(4个)`${system:user.name}`替换为`root`

3. 进入hive根目录，执行

    ```shell
    schematool -initSchema -dbType derby
    ```
    - 上述命令执行完毕后会在对应目录下新建metastore_db目录，用于存储数据目录
    - derby是hive自带的小数据库，后续需要将derby更换成mysql(TODO)

4. 在该目录下启动执行`hive`

   **注意：**
   - hive命令执行时，必须与metastore_db在同一目录下
   - hive启动前需要将hdfs也启动，不然会报错

5. hive连接mysql

    - 关于虚拟机安装了mysql数据库，主机无法连接的问题如下：

    ```plsql
    mysql> use mysql;
    Database changed
    mysql> select 'host' from user where user='root'
        -> ;
    +------+
    | host |
    +------+
    | localhost |
    +------+
    1 row in set (0.00 sec)

    mysql> update user set host = '%' where user ='root';
    Query OK, 1 row affected (0.00 sec)
    Rows matched: 1  Changed: 1  Warnings: 0

    mysql> flush privileges;
    Query OK, 0 rows affected (0.01 sec)

    mysql> select 'host'   from user where user='root';
    +------+
    | host |
    +------+
    |  %   |
    +------+
    1 row in set (0.00 sec)
    ```

    - 解决jdbc连接hive时出现`Open Session Error`

    ```xml
    <property>
        <name>hadoop.proxyuser.hadoop.hosts</name>
        <value>*</value>
    </property>
    <property>
        <name>hadoop.proxyuser.hadoop.groups</name>
        <value>*</value>
    </property>
    ```

6. hive表存储格式

   1. TextFile
   2. SequenceFile
   3. RCFile
   4. ORCFile

# HA的实现

## 集群环境

- Hadoop 2.9.2
- Hive 2.3.4
- MySQL 5.7
- Zookeeper 3.4.10
- JDK 8u191

## 集群结构图

> 共9台机器，其中3台ZooKeeper集群，5台Hadoop集群（2台Master，3台Slave），1台应用服务器

| 主机名 | IP | 软件 | 运行进程 |
| :--------: | :--------:| :--: | :--: |
| node0 | 192.168.137.200 | ZooKeeper | QuorumPeerMain |
| node1 | 192.168.137.201 | ZooKeeper | QuorumPeerMain |
| node2 | 192.168.137.202 | ZooKeeper | QuorumPeerMain |
| master | 192.168.137.100 | Hadoop,Hive,MySql | JournalNode,NameNode,ResourceManager,<br/>DFSZKFailoverController,HiveServer2,MySql |
| master1 | 192.168.137.10 | Hadoop,Hive | JournalNode,NameNode,ResourceManager,<br/>DFSZKFailoverController,HiveServer2 |
| slave1 | 192.168.137.101 | Hadoop | JournalNode,DataNode,NodeManager |
| slave2 | 192.168.137.102 | Hadoop | DataNode,NodeManager |
| slave3 | 192.168.137.103 | Hadoop | DataNode,NodeManager |
| host | 192.168.137.1 | 应用服务器 | |

## 集群启动步骤

```shell
# 在三台zookeeper上启动zkServer
zkServer.sh start

# master上执行hdfs和yarn集群的启动
start-dfs.sh
start-yarn.sh

# master1上的ResourceManager不知道为何不会自动启动，因此手动
yarn-daemon.sh start resourcemanager

# master和master1上都要启动hiveserver2
hiveserver2
```

## 关于[二级缓存](http://www.datanucleus.org/products/accessplatform_3_0/jpa/cache.html)的若干事宜

在集群启动完毕之后，我在master的hive中使用`create table test(...)`语句创建了一个表并导入了一些数据，而后通过应用执行该表的相关查询后，发现时不时提示表`test`不存在，通过hive查看master和master1的表结构，发现master1中竟然没有表test，查阅资料后看到一篇博文[《hive datanucleus cache 不一致问题》](https://blog.csdn.net/lalaguozhe/article/details/9184593)，是关于hive中的DataNucleus二级缓存的设置，`datanucleus.cache.level2.type`的设置(`none`,`soft`,`weak`)直接影响二级缓存是否启用，关于二级缓存的具体机制我还没弄清楚，之后我又在[Hortonworks](https://docs.hortonworks.com/HDPDocuments/HDP2/HDP-2.6.5/bk_command-line-installation/content/set_up_hive_hcat_configuration_files.html)的文档中看到下面的话：

> **Important**:
> Hortonworks recommends that deployments <font color=red>**disable the DataNucleus cache**</font> by setting the value of the datanucleus.cache.level2.type configuration parameter to none. The datanucleus.cache.level2 configuration parameter is ignored, and assigning a value of none to this parameter does not have the desired effect.

可能是Hiveserver2本身在HA的层面就不建议修改库、表结构，因此若要更改表结构或者创建新表同时实现数据同步，我试了以下两种方式均可实现：

1. 第一种关闭并重新初始化集群，启动master和所有的DataNode，在master上执行建库建表导入数据，而后启动master1将其作为standby初始化，这时master1会同步master的数据，最后启动master和master1的hiveserver2

2. 第二种是同时在master和master1的hive客户端上执行同样的建表语句，而后在master(或者master1)上执行`load`命令加载数据，即可同步数据

    > 第二种方法又出现了个有趣的问题：导入数据完成后，在两台机器上分别执行`count`操作会发现由于加载数据的机器count正常，另一台机器count结果为0，但是执行`select *`又确实能发现数据存在，就很玄学。

# TODO List

- 安全与权限（kerberos）
- Secondary NameNode（check point NameNode）
- HA（High Ability）实现
- Federation，超大规模数据中心