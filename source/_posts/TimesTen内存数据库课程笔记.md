---
title: TimesTen内存数据库课程笔记（更新中）
date: 2018-9-2 20:59:50
tags: 
    - TimesTen 
    - 内存数据库 
    - 大三上笔记

categories: 
    - archives
---
# 内存计算与内存数据库

## 第零章

OLTP：行存储（记录：元组），联机事务处理

OLAP：列存储（key-value），联机分析处理

## Timesten操作小记

<!-- more -->

### 平台

> 系统：Red Hat Enterprise Linux Server release 5.7 (Tikanga)
>

### 创建DSN（Data Source Name）

> 逻辑名，用于标识某一数据库连接

#### 打开数据库配置文件(通常称为`系统ODBC.INI配置文件`)

```sh
$ cd $TT_HOME/info
$ gedit sys.odbc.ini
```
#### 在数据库DSN列表中添加需要新建的数据库名称

```ini
# 添加my_ttdb数据库，“=”后面是指该数据库使用某种驱动，如第3行所示
[ODBC Data Sources]
my_ttdb=TimesTen 11.2.2 Driver
TT_1122=TimesTen 11.2.2 Driver
sampledb_1122=TimesTen 11.2.2 Driver
cachedb1_1122=TimesTen 11.2.2 Driver
repdb1_1122=TimesTen 11.2.2 Driver
repdb2_1122=TimesTen 11.2.2 Driver
sampledbCS_1122=TimesTen 11.2.2 Client Driver
cachedb1CS_1122=TimesTen 11.2.2 Client Driver
repdb1CS_1122=TimesTen 11.2.2 Client Driver
repdb2CS_1122=TimesTen 11.2.2 Client Driver
```

#### 为2中创建的数据库添加配置，<u>**日志文件与检查点文件应存储在不同磁盘中**</u>

```ini
# 配置my_ttdb
[my_ttdb]
# 数据库监听器驱动位置
Driver=/home/oracle/TimesTen/tt1122/lib/libtten.so 
# DataStore为检查点文件存储位置
DataStore=/u02/ttdata/datastores/my_ttdb 
# LogDir为日志文件存储位置
LogDir=/u03/ttdata/logs
# 以下两个Size是TimesTen内存数据库的内存分配
PermSize=40
TempSize=32
# 数据库的字符集
DatabaseCharacterSet=AL32UTF8
```

> TimesTen的内存分配主要是PermSize和TempSize两块，可先参考博客[**<u>如何更改TimesTen数据库的大小</u>**](https://blog.csdn.net/stevensxiao/article/details/51050831)。

#### 保存配置文件并关闭

### 数据库服务器基本命令

#### 查看服务器状态

```sh
[oracle@timesten-hol info]$ ttstatus
TimesTen status report as of Thu Sep 27 04:08:30 2018

Daemon pid 2637 port 53392 instance tt1122
TimesTen server pid 2646 started on port 53393
------------------------------------------------------------------------
Accessible by group oracle
End of report
```

#### 启动/停止数据库

```sh
[oracle@timesten-hol info]$ ttdaemonadmin -stop
TimesTen Daemon stopped.
[oracle@timesten-hol info]$ ttstatus
ttStatus: Could not connect to the TimesTen daemon.
If the TimesTen daemon is not running, please start it
by running "ttDaemonAdmin -start".
[oracle@timesten-hol info]$ ttdaemonadmin -start
TimesTen Daemon startup OK.
[oracle@timesten-hol info]$ ttstatus
TimesTen status report as of Thu Sep 27 04:10:00 2018

Daemon pid 6522 port 53392 instance tt1122
TimesTen server pid 6531 started on port 53393
------------------------------------------------------------------------
Accessible by group oracle
End of report
```

### 创建TimesTen内存数据库

> 默认情况下，TimesTen内存数据库在第一次连接到数据库时创建并加载到内存中，并在关闭数据库的最后一个连接时从内存卸载。当然此行为可通过`ttadmin -RAMPolicy`修改，后面会说到。
>
> 也就是说，默认情况下（前提是RAM策略为`inUse`，下一节会讲到RAM策略的修改），每次在执行`connect “dsn=ttdb_name”`连接到一个特定的DSN时，都是一个创建TimesTen内存数据库、加载数据到内存中等过程，因此本节的标题是`创建`而不是`连接到`。

#### 连接到特定DSN，创建内存数据库

```sh
[oracle@timesten-hol info]$ ttisql

Copyright (c) 1996, 2014, Oracle and/or its affiliates. All rights reserved.
Type ? or "help" for help, type "exit" to quit ttIsql.

Command> connect "dsn=my_ttdb";
Connection successful: DSN=my_ttdb;UID=oracle;DataStore=/u02/ttdata/datastores/my_ttdb;DatabaseCharacterSet=AL32UTF8;ConnectionCharacterSet=US7ASCII;DRIVER=/home/oracle/TimesTen/tt1122/lib/libtten.so;LogDir=/u03/ttdata/logs;PermSize=40;TempSize=32;TypeMode=0;
(Default setting AutoCommit=1)
```

或者直接在ttisql中指定DSN名称：

```sh
[oracle@timesten-hol info]$ ttisql "dsn=my_ttdb"

Copyright (c) 1996, 2014, Oracle and/or its affiliates. All rights reserved.
Type ? or "help" for help, type "exit" to quit ttIsql.

connect "dsn=my_ttdb";
Connection successful: DSN=my_ttdb;UID=oracle;DataStore=/u02/ttdata/datastores/my_ttdb;DatabaseCharacterSet=AL32UTF8;ConnectionCharacterSet=US7ASCII;DRIVER=/home/oracle/TimesTen/tt1122/lib/libtten.so;LogDir=/u03/ttdata/logs;PermSize=40;TempSize=32;TypeMode=0;
(Default setting AutoCommit=1)
[oracle@timesten-hol ~]$ ttisql my_ttdb

Copyright (c) 1996, 2014, Oracle and/or its affiliates. All rights reserved.
Type ? or "help" for help, type "exit" to quit ttIsql.

connect "DSN=my_ttdb";
Connection successful: DSN=my_ttdb;UID=oracle;DataStore=/u02/ttdata/datastores/my_ttdb;DatabaseCharacterSet=AL32UTF8;ConnectionCharacterSet=US7ASCII;DRIVER=/home/oracle/TimesTen/tt1122/lib/libtten.so;LogDir=/u03/ttdata/logs;PermSize=40;TempSize=32;TypeMode=0;
(Default setting AutoCommit=1)
```



> **问题：重复运行`connect “dsn=ttdb_name”`命令可以看到命令行中显示了多了连接，这是什么作用呢？**
>
> ```sh
> Command> connect "dsn=my_ttdb";
> Connection successful: DSN=my_ttdb;UID=oracle;DataStore=/u02/ttdata/datastores/my_ttdb;DatabaseCharacterSet=AL32UTF8;ConnectionCharacterSet=US7ASCII;DRIVER=/home/oracle/TimesTen/tt1122/lib/libtten.so;LogDir=/u03/ttdata/logs;PermSize=40;TempSize=32;TypeMode=0;
> (Default setting AutoCommit=1)
> Command> connect "dsn=my_ttdb";
> Connection successful: DSN=my_ttdb;UID=oracle;DataStore=/u02/ttdata/datastores/my_ttdb;DatabaseCharacterSet=AL32UTF8;ConnectionCharacterSet=US7ASCII;DRIVER=/home/oracle/TimesTen/tt1122/lib/libtten.so;LogDir=/u03/ttdata/logs;PermSize=40;TempSize=32;TypeMode=0;
> (Default setting AutoCommit=1)
> con1: Command> connect "dsn=my_ttdb";
> Connection successful: DSN=my_ttdb;UID=oracle;DataStore=/u02/ttdata/datastores/my_ttdb;DatabaseCharacterSet=AL32UTF8;ConnectionCharacterSet=US7ASCII;DRIVER=/home/oracle/TimesTen/tt1122/lib/libtten.so;LogDir=/u03/ttdata/logs;PermSize=40;TempSize=32;TypeMode=0;
> (Default setting AutoCommit=1)
> con2: Command> connect "dsn=my_ttdb";
> Connection successful: DSN=my_ttdb;UID=oracle;DataStore=/u02/ttdata/datastores/my_ttdb;DatabaseCharacterSet=AL32UTF8;ConnectionCharacterSet=US7ASCII;DRIVER=/home/oracle/TimesTen/tt1122/lib/libtten.so;LogDir=/u03/ttdata/logs;PermSize=40;TempSize=32;TypeMode=0;
> (Default setting AutoCommit=1)
> con3: Command> 
> ```

#### 查看内存数据库的内存分配及容量

```sh
Command> dssize

  PERM_ALLOCATED_SIZE:      40960
  PERM_IN_USE_SIZE:         9453
  PERM_IN_USE_HIGH_WATER:   9453
  TEMP_ALLOCATED_SIZE:      32768
  TEMP_IN_USE_SIZE:         9442
  TEMP_IN_USE_HIGH_WATER:   9505
```

#### 使用Host命令可以调用操作系统级别的指令

```sh
Command> host ttstatus;
TimesTen status report as of Thu Sep 27 04:37:28 2018

Daemon pid 6522 port 53392 instance tt1122
TimesTen server pid 6531 started on port 53393
------------------------------------------------------------------------
Data store /u01/ttdata/datastores/my_ttdb
There are 12 connections to the data store
Shared Memory KEY 0x1200c904 ID 2785297
PL/SQL Memory KEY 0x1300c904 ID 2818066 Address 0x7fa0000000
Type            PID     Context             Connection Name              ConnID
Process         6973    0x0000000000c72c00  my_ttdb                           1
Subdaemon       6529    0x00000000012d3360  Manager                         142
Subdaemon       6529    0x000000000132a1e0  Rollback                        141
Subdaemon       6529    0x000000000140b360  HistGC                          139
Subdaemon       6529    0x0000000001420070  AsyncMV                         140
Subdaemon       6529    0x00000000014b4e00  Log Marker                      136
Subdaemon       6529    0x0000000001509a30  Deadlock Detector               135
Subdaemon       6529    0x000000000151e620  Flusher                         134
Subdaemon       6529    0x0000000001533210  Checkpoint                      133
Subdaemon       6529    0x00000000016286b0  Monitor                         132
Subdaemon       6529    0x00007f95880208e0  Aging                           138
Subdaemon       6529    0x00007f958808f900  IndexGC                         137
Replication policy  : Manual
Cache Agent policy  : Manual
PL/SQL enabled.
------------------------------------------------------------------------
Accessible by group oracle
End of report
```

### 修改RAM策略

> 上一节讲到每一次的连接到特定的DSN都是新建一个内存数据库的过程，当然这是基于默认RAM策略为`inUse`的情况，下面会讲到当RAM策略设置为`Manual`时创建内存数据库的过程。
>
> `Manual`策略适用于当数据库中数据规模巨大，装载到内存中的时间可能很长，从而导致内存数据库效率低下；而`inUse`策略适用于大多数情况，数据规模不是很大，装载到内存中的时间很短或者说在业务需求中可以忽略不计。

#### 查看当前RAM策略

```sh
[oracle@timesten-hol info]$ ttadmin my_ttdb
RAM Residence Policy            : inUse
Replication Agent Policy        : manual
Replication Manually Started    : False
Cache Agent Policy              : manual
Cache Agent Manually Started    : False
```

#### 修改RAM策略为手动模式（Manual）

> 手动模式下，创建DSN连接时并不会将数据加载到内存中，需要手动进行数据装载和卸载

```sh
[oracle@timesten-hol info]$ ttadmin -rampolicy manual my_ttdb
RAM Residence Policy            : manual
Manually Loaded In RAM          : False
Replication Agent Policy        : manual
Replication Manually Started    : False
Cache Agent Policy              : manual
Cache Agent Manually Started    : False
[oracle@timesten-hol info]$ ttisql "dsn=my_ttdb";

Copyright (c) 1996, 2014, Oracle and/or its affiliates. All rights reserved.
Type ? or "help" for help, type "exit" to quit ttIsql.

connect "dsn=my_ttdb";
  707: Attempt to connect to a data store that has been manually unloaded from RAM
The command failed.
Done.
[oracle@timesten-hol info]$ 
```

#### 向内存中装载数据

```sh
[oracle@timesten-hol info]$ ttadmin -ramload my_ttdb
RAM Residence Policy            : manual
Manually Loaded In RAM          : True
Replication Agent Policy        : manual
Replication Manually Started    : False
Cache Agent Policy              : manual
Cache Agent Manually Started    : False
[oracle@timesten-hol info]$ ttisql "dsn=my_ttdb";

Copyright (c) 1996, 2014, Oracle and/or its affiliates. All rights reserved.
Type ? or "help" for help, type "exit" to quit ttIsql.

connect "dsn=my_ttdb";
Connection successful: DSN=my_ttdb;UID=oracle;DataStore=/u02/ttdata/datastores/my_ttdb;DatabaseCharacterSet=AL32UTF8;ConnectionCharacterSet=US7ASCII;DRIVER=/home/oracle/TimesTen/tt1122/lib/libtten.so;LogDir=/u03/ttdata/logs;PermSize=40;TempSize=32;TypeMode=0;
(Default setting AutoCommit=1)
Command> 
```

#### 从内存中卸载数据

```sh
[oracle@timesten-hol info]$ ttadmin -ramunload my_ttdb
RAM Residence Policy            : manual
Manually Loaded In RAM          : False
Replication Agent Policy        : manual
Replication Manually Started    : False
Cache Agent Policy              : manual
Cache Agent Manually Started    : False
[oracle@timesten-hol info]$ ttisql "dsn=my_ttdb";

Copyright (c) 1996, 2014, Oracle and/or its affiliates. All rights reserved.
Type ? or "help" for help, type "exit" to quit ttIsql.

connect "dsn=my_ttdb";
  707: Attempt to connect to a data store that has been manually unloaded from RAM
The command failed.
Done.
[oracle@timesten-hol info]$ 
```

### 日志和检查点

#### 查看日志文件，**<u>提交之前会预写日志</u>**

```sh
Command> host ls -al /u03/ttdata/logs/my*
-rw-rw---- 1 oracle oracle 18270208 Sep 28 23:00 /u03/ttdata/logs/my_ttdb.log4
-rw-rw---- 1 oracle oracle 67108864 Sep 27 04:18 /u03/ttdata/logs/my_ttdb.res0
-rw-rw---- 1 oracle oracle 67108864 Sep 27 04:18 /u03/ttdata/logs/my_ttdb.res1
-rw-rw---- 1 oracle oracle 67108864 Sep 27 04:18 /u03/ttdata/logs/my_ttdb.res2
```

#### 查看检查点

```sh
Command> host ls -al /u02/ttdata/datastores/my*
-rw-rw---- 1 oracle oracle 31906840 Sep 28 23:00 /u02/ttdata/datastores/my_ttdb.ds0
-rw-rw---- 1 oracle oracle 31906840 Sep 28 22:57 /u02/ttdata/datastores/my_ttdb.ds1
```

#### 手动更新检查点文件

> 非手动状态下检查点会每间隔一段时间执行一次，会将自上次检查点后提交的事务更新到检查点中；检查点文件是非阻塞的，即更新检查点文件时也可执行事务。
>
> 如下调用检查点文件：

```plsql
Command> call ttckpt;
Command> call ttckpt;
```

### ttisql基本命令——用户操作

#### 创建用户，可在表`sys.all_users`中查找所有的用户信息

```plsql
Command> select * from sys.all_users;
< SYS, 0, 2018-09-27 04:18:18.063030 >
< TTREP, 2, 2018-09-27 04:18:18.063030 >
< SYSTEM, 3, 2018-09-27 04:18:18.063030 >
< GRID, 4, 2018-09-27 04:18:18.063030 >
< ORACLE, 10, 2018-09-27 04:18:18.063030 >
< SCOTT, 11, 2018-09-27 05:06:39.267433 >
6 rows found.
Command> create user tthr identified by tthr;

User created.

Command> select * from sys.all_users;
< SYS, 0, 2018-09-27 04:18:18.063030 >
< TTREP, 2, 2018-09-27 04:18:18.063030 >
< SYSTEM, 3, 2018-09-27 04:18:18.063030 >
< GRID, 4, 2018-09-27 04:18:18.063030 >
< ORACLE, 10, 2018-09-27 04:18:18.063030 >
< SCOTT, 11, 2018-09-27 05:06:39.267433 >
< TTHR, 12, 2018-09-28 23:11:57.126074 >
7 rows found.
```

#### 给用户分配权限

```plsql
Command> grant create session to tthr;
Command> grant create table to tthr;
Command> grant create view to tthr;
Command> grant create sequence to tthr;
```

#### 查看当前数据库系统内用户权限

```
Command> select * from sys.dba_sys_privs;
< SYS, ADMIN, YES >
< SYSTEM, ADMIN, YES >
< ORACLE, ADMIN, YES >
< SCOTT, CREATE SESSION, NO >
< SCOTT, CREATE TABLE, NO >
< TTHR, CREATE SESSION, NO >
< TTHR, CREATE TABLE, NO >
< TTHR, CREATE VIEW, NO >
< TTHR, CREATE SEQUENCE, NO >
9 rows found.
```

#### 撤回用户权限

> 以下示例展示了如何从用户撤回权限（赋予`delete any table`权限后再撤回该权限）

```plsql
Command> grant delete any table to tthr;
Command> select * from sys.dba_sys_privs;
< SYS, ADMIN, YES >
< SYSTEM, ADMIN, YES >
< ORACLE, ADMIN, YES >
< SCOTT, CREATE SESSION, NO >
< SCOTT, CREATE TABLE, NO >
< TTHR, CREATE SESSION, NO >
< TTHR, DELETE ANY TABLE, NO >
< TTHR, CREATE TABLE, NO >
< TTHR, CREATE VIEW, NO >
< TTHR, CREATE SEQUENCE, NO >
10 rows found.
Command> revoke delete any table from tthr;
Command> select * from sys.dba_sys_privs;
< SYS, ADMIN, YES >
< SYSTEM, ADMIN, YES >
< ORACLE, ADMIN, YES >
< SCOTT, CREATE SESSION, NO >
< SCOTT, CREATE TABLE, NO >
< TTHR, CREATE SESSION, NO >
< TTHR, CREATE TABLE, NO >
< TTHR, CREATE VIEW, NO >
< TTHR, CREATE SEQUENCE, NO >
9 rows found.
```

### ttisql基本命令——数据库对象操作

#### 关闭自动提交

> 意即每次执行事务后，均需要执行`commit`以提交事务。

```plsql
Command> autocommit off;
```

#### 建表、插入数据

```plsql
Command> create table ttemployees
       > (employee_id NUMBER(6) NOT NULL,
       > last_name VARCHAR2(10) NOT NULL, hire_date DATE, performance_report CLOB,
       > PRIMARY KEY (employee_id) )
       > UNIQUE HASH ON (employee_id) PAGES = 1;
Command> insert into ttemployees values (1, 'Smith', '2009-02-23', 'excellent'); 
1 row inserted.
Command> insert into ttemployees values (2, 'King', '2005-08-05', 'great');
1 row inserted.
Command> insert into ttemployees values (3, 'Taylor', '2012-01-28', EMPTY_CLOB());
1 row inserted.
Command> commit;
```

#### 一些命令总结

> - tables and alltables - Lists tables
> - indexes and allindexes - Lists indexes
> - views and allviews - Lists views
> - sequences and allsequences - Lists sequences
> - synonyms and allsynonyms - Lists synonyms
> - functions and allfunctions - Lists PL/SQL functions
> - procedures and allprocedures - Lists PL/SQL procedures
> - packages and allpackages - Lists PL/SQL packages

### PLSQL编程

#### 创建plsqldb、pls用户、运行sql脚本

```sql
call ttOptUpdateStats;
// 更新统计数据，用于分析生成最优执行计划
```

#### 使用sql developer连接TimesTen和Oracle

配置如下：

![TimesTen数据库连接配置](/images/image-20181018140355729.png)

![Oracle数据库连接配置](/images/image-20181018140412582.png)

#### plsql语法

> ## What Is a PL/SQL Package?
>
> A **package** is a schema object that groups logically related PL/SQL types, items, and subprograms. Packages usually have two parts, a specification and a body, although sometimes the body is unnecessary. The **specification** (**spec** for short) is the interface to your applications; it declares the types, variables, constants, exceptions, cursors, and subprograms available for use. The **body** fully defines cursors and subprograms, and so implements the spec.
>
> `包`是一个模式对象，它对逻辑上相关的PL/SQL类型、项和子程序进行分组。包通常有两个部分，`规范`和`主体`，主体不是必要的。`规范`是应用程序的接口：它声明可用的类型、变量、常量、异常、游标和子程序。`主体`将完全定义游标和子程序，以此实现`规范`。
>
> As [Figure 9-1](https://docs.oracle.com/cd/B10501_01/appdev.920/a96624/09_packs.htm#5871) shows, you can think of the spec as an operational interface and of the body as a "black box." You can debug, enhance, or replace a package body without changing the interface (package spec) to the package.
>
> ![包](/images/image-20181018143452887.png)
>
> ——[Oracle PL/SQL Package文档](https://docs.oracle.com/cd/B10501_01/appdev.920/a96624/09_packs.htm#362)

`1_package.sql:`

```plsql
CREATE OR REPLACE PACKAGE test AS

  -- Declare a record for the desired EMP fields
  TYPE empRecType IS RECORD (
    r_empno  EMP.EMPNO%TYPE,
      -- 使用EMP表中EMPNO的类型
    r_ename  EMP.ENAME%TYPE,
    r_salary EMP.SAL%TYPE
  );

  -- Declare a Ref Cursor type
  TYPE EmpCurType IS REF CURSOR RETURN empRecType; -- 游标类型需要有返回值

  -- A parameterized cursor，定义
  	-- 游标
  CURSOR low_paid (num PLS_INTEGER) IS
    SELECT empno 
      FROM emp
      WHERE rownum <= num
      ORDER BY sal ASC;
	-- 过程(IN表示输入，OUT表示输出)
  PROCEDURE ddl_dml
    (myComment IN  VARCHAR2,
     errCode   OUT PLS_INTEGER, -- 整型
     errText   OUT VARCHAR2); 


  PROCEDURE givePayRise
    (num       IN  PLS_INTEGER,
     name      OUT EMP.ENAME%TYPE, 
     	-- name是plsql中的保留字，应该尽量避免使用保留字
     errCode   OUT PLS_INTEGER,
     errText   OUT VARCHAR2); 


  PROCEDURE getCommEmps
    (empRefCur IN OUT EmpCurType,
     errCode   OUT PLS_INTEGER,
     errText   OUT VARCHAR2); 

  -- Associative array
  TYPE sum_multiples IS TABLE OF PLS_INTEGER -- Associative array type
  INDEX BY PLS_INTEGER; -- indexed by pls_integer
  
  FUNCTION get_sum_multiples
   ( multiple IN PLS_INTEGER,
     num      IN PLS_INTEGER
   ) RETURN sum_multiples;

END test;
/


CREATE OR REPLACE PACKAGE BODY test AS

  PROCEDURE ddl_dml
    (myComment IN  VARCHAR2,
     errCode   OUT PLS_INTEGER,
     errText   OUT VARCHAR2) IS

    sql_str                    VARCHAR2(256);
    name_already_exists        EXCEPTION;
    insufficient_privileges    EXCEPTION;
    PRAGMA EXCEPTION_INIT(name_already_exists,     -0955);
    PRAGMA EXCEPTION_INIT(insufficient_privileges, -1031);
    seq_value                  number;


  BEGIN


    BEGIN
      sql_str := 'create table foo (COL1 VARCHAR2 (20),COL2 NVARCHAR2 (60))';
      DBMS_OUTPUT.PUT_LINE(sql_str);
      execute immediate sql_str;
    EXCEPTION
      WHEN name_already_exists THEN
        DBMS_OUTPUT.PUT_LINE('  Ignore existing table errors');
      WHEN insufficient_privileges THEN
        DBMS_OUTPUT.PUT_LINE('  Ignore insufficient privileges errors');
    END;

    -- Cast num_col1 and char_col values
    insert into temp values (1, 1, myComment);

    commit;

    errCode := 0;
    errtext := 'OK';

  EXCEPTION
  
    WHEN name_already_exists THEN

      errCode := 0;
      errtext := 'OK';

    WHEN OTHERS THEN

      errCode  := SQLCODE;
      errText  := SUBSTR(SQLERRM, 1, 200);

  END ddl_dml;



  PROCEDURE givePayRise
    (num       IN  PLS_INTEGER,
     name      OUT EMP.ENAME%TYPE,
     errCode   OUT PLS_INTEGER,
     errText   OUT VARCHAR2) IS

   -- Can use PLSQL collections within TimesTen PLSQL
   TYPE lowest_paid_type IS TABLE OF emp.empno%TYPE;
   lowest_paid lowest_paid_type;

   i           PLS_INTEGER; 
   numRows     PLS_INTEGER;
   lucky_index PLS_INTEGER; 
   lucky_emp   EMP.EMPNO%TYPE; 

  BEGIN

    -- Initialize the output variable
    name := 'Nobody';

    -- Initialize the collection
    lowest_paid := lowest_paid_type(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    i := 1;
    
    -- Constrain the resultset size
    IF num < 1 OR num > 10 THEN

      -- If bad inputs, default to 5 rows
      numRows := 5;
    ELSE
      numRows := num;
    END IF;


    -- Create the cursor resultset with up to 'numRows' rows
    OPEN low_paid( numRows );

    LOOP

      -- Get the current empid
      FETCH low_paid INTO lowest_paid(i);

      EXIT WHEN low_paid%NOTFOUND;

      -- Increment the PLSQL table index
      i := i + 1;

    END LOOP;

    -- Close the cursor
    CLOSE low_paid;


    -- List the subset of lowest paid employees
    FOR j in lowest_paid.FIRST .. numRows LOOP
      DBMS_OUTPUT.PUT_LINE('  Lowest paid empno ' || j || ' is ' || lowest_paid(j) );
    END LOOP;

    -- Randomly choose one of the lowest paid employees for a 10% pay raise.
    lucky_index := trunc(dbms_random.value(lowest_paid.FIRST, numRows)); 
    lucky_emp := lowest_paid(lucky_index);


    -- Give lucky_emp a 10% pay raise and return their name
    UPDATE emp
      SET sal = sal * 1.1
      WHERE empno = lucky_emp
      RETURNING ename INTO name;

    COMMIT;

    errCode := 0;
    errtext := 'OK';

  EXCEPTION
  
    WHEN OTHERS THEN

      errCode  := SQLCODE;
      errText  := SUBSTR(SQLERRM, 1, 200);

  END givePayRise;



  PROCEDURE getCommEmps
    (empRefCur IN OUT EmpCurType,
     errCode   OUT PLS_INTEGER,
     errText   OUT VARCHAR2) IS

    salesGuy empRecType;

  BEGIN 

    DBMS_OUTPUT.PUT_LINE(' ');
    DBMS_OUTPUT.PUT_LINE('Displaying the refcursor for the sales people');

    -- The refcursor (empRefCur) result was opened before calling this procedure
    LOOP
      FETCH empRefCur INTO salesGuy;
      EXIT WHEN empRefCur%NOTFOUND;

      DBMS_OUTPUT.PUT_LINE(salesGuy.r_ename);
    END LOOP;

    CLOSE empRefCur;

    errCode := 0;
    errtext := 'OK';

  EXCEPTION

  
    WHEN OTHERS THEN

      errCode  := SQLCODE;
      errText  := SUBSTR(SQLERRM, 1, 200);

  END getCommEmps;

  FUNCTION get_sum_multiples
   ( multiple IN PLS_INTEGER,
     num      IN PLS_INTEGER
   ) RETURN sum_multiples
   IS
     s sum_multiples;
  BEGIN
    FOR i in 1..num LOOP
      s(i) := multiple * ((i * (i + 1)) / 2); -- sum of the multiples
    END LOOP;
    RETURN s;
  END get_sum_multiples;

BEGIN  -- package initialization goes here
  DBMS_OUTPUT.PUT_LINE('Initialized package test');
END test;
/
```

`2_call_package.sql:`

```plsql
set serveroutput on;

declare
  errCode      PLS_INTEGER;
  errtext      VARCHAR2(256);
  myRefCur     test.EmpCurType; -- 使用test包中定义的类型
  salesPerson  test.empRecType;
  name         EMP.ENAME%TYPE;
  n           PLS_INTEGER := 5; -- number of multiples to sum for display
  sn          PLS_INTEGER := 10; -- number of multiples to sum
  m           PLS_INTEGER := 3; -- multiple
  
  
begin

    dbms_output.put_line(' ');
    dbms_output.put_line(' ');
    dbms_output.put_line(' ');
    dbms_output.put_line('Find some of the lowest paid employees and give a random employee a 10% pay raise');
    -- Give a lowely paid random employee a 10% pay raise
    test.givePayRise(5, name, errCode, errText);
    if errCode != 0 then
      dbms_output.put_line('Error code = ' || errCode || ' Error Text = ' || errtext);
    else
      dbms_output.put_line(name || ' got the 10% payraise');
    end if;

    -- Open a refcursor
    OPEN myRefCur FOR
      SELECT empno, ename, sal
      FROM emp
      WHERE comm IS NOT NULL;

    -- display the resultset of the refcursor
    test.getCommEmps(myRefCur, errCode, errText);
    if errCode != 0 then
      dbms_output.put_line('Error code = ' || errCode || ' Error Text = ' || errtext);
    end if;

    dbms_output.put_line(' ');
    dbms_output.put_line('Do some DDL and DML in a stored procedure');
    test.ddl_dml('hi', errCode, errText);
    if errCode != 0 then
      dbms_output.put_line('Error code = ' || errCode || ' Error Text = ' || errtext);
    end if;
    
    -- associative arrays
    dbms_output.put_line(' ');
    dbms_output.put_line('Use an associative array to compute the sum of multiples');
    dbms_output.put_line(
      'Sum of the first ' || TO_CHAR(n) || ' multiples of ' || TO_CHAR(m) 
       || ' is ' ||  TO_CHAR(test.get_sum_multiples (m, sn)(n)));
    

end;
/
```

`3_create_package_workload.sql:`

```plsql
CREATE OR REPLACE PACKAGE workload AS

  PROCEDURE oltp_read_only (
    v_id      IN  PLS_INTEGER,
    v_n       IN  PLS_INTEGER,
    v_m       IN  PLS_INTEGER,
    errCode   OUT PLS_INTEGER,
    errText   OUT VARCHAR2);

  PROCEDURE oltp_read_write (
    v_id      IN  PLS_INTEGER,
    v_n       IN  PLS_INTEGER,
    v_m       IN  PLS_INTEGER,
    v_c       IN  CHAR,
    v_p       IN  VARCHAR2,
    errCode   OUT PLS_INTEGER,
    errText   OUT VARCHAR2);

END workload;
/


CREATE OR REPLACE PACKAGE BODY workload AS

  -- Private package variables used for package initialization
  theErrCode PLS_INTEGER   := 0;
  theErrText VARCHAR2(256) := 'OK';

  -- Using shared package cursors for efficiency
  CURSOR range_query (n PLS_INTEGER, m PLS_INTEGER) IS
     SELECT c 
       FROM sbtest 
       WHERE id BETWEEN n AND m;

  CURSOR range_order_query (n PLS_INTEGER, m PLS_INTEGER) IS
     SELECT c 
       FROM sbtest 
       WHERE id BETWEEN n AND m
       ORDER BY c;

  CURSOR range_distinct_query (n PLS_INTEGER, m PLS_INTEGER) IS
     SELECT DISTINCT c 
       FROM sbtest 
       WHERE id BETWEEN n AND m
       ORDER BY c;


  -- The workload read only workload
  PROCEDURE oltp_read_only (
    v_id      IN  PLS_INTEGER,
    v_n       IN  PLS_INTEGER,
    v_m       IN  PLS_INTEGER,
    errCode   OUT PLS_INTEGER,
    errText   OUT VARCHAR2) IS

    -- Store the result of the column 'c'
    cValue  char(120);

    -- Store the sum of the rows in (n..m)
    sumK    number(38,0);

  BEGIN

    errCode := 0;
    errtext := 'OK';

    -- oltp point query
    FOR i in 1 .. 10 LOOP

      -- DBMS_OUTPUT.PUT_LINE('oltp point query');
      SELECT c INTO cValue FROM sbtest WHERE id = v_id;
      -- DBMS_OUTPUT.PUT_LINE('c = ' || cValue);

    END LOOP;

    -- oltp range query (using a cursor for loop)
--    DBMS_OUTPUT.PUT_LINE('oltp range query');
    FOR range_rows IN range_query(v_n, v_m)
    LOOP
--      DBMS_OUTPUT.PUT_LINE(range_query%ROWCOUNT || ' c = ' || range_rows.c);
      null;
    END LOOP;

    -- olpt range SUM() query
--    DBMS_OUTPUT.PUT_LINE('oltp range SUM() query');
    SELECT sum(k) INTO sumK FROM sbtest WHERE id BETWEEN v_n AND v_m;
--    DBMS_OUTPUT.PUT_LINE('sumK = ' || sumK);

    -- oltp range ORDER BY query (using explicit fetches)
--    DBMS_OUTPUT.PUT_LINE('oltp range ORDER BY query');
    OPEN range_order_query(v_n, v_m);
    LOOP
      FETCH range_order_query INTO cValue;
      EXIT WHEN range_order_query%NOTFOUND;
--      DBMS_OUTPUT.PUT_LINE('c = ' || cValue);
    END LOOP;
    CLOSE range_order_query;

    -- oltp range DISTINCT query
--    DBMS_OUTPUT.PUT_LINE('oltp range DISTINCT query');
    OPEN range_distinct_query(v_n, v_m);
    LOOP
      FETCH range_distinct_query INTO cValue;
      EXIT WHEN range_distinct_query%NOTFOUND;
--      DBMS_OUTPUT.PUT_LINE('c = ' || cValue);
    END LOOP;
    CLOSE range_distinct_query;

  EXCEPTION

    WHEN NO_DATA_FOUND THEN

      errCode  := 0;
      errText  := 'OK';

    WHEN OTHERS THEN

      errCode  := SQLCODE;
      errText  := SUBSTR(SQLERRM, 1, 200);

  END oltp_read_only;


  -- The workload read + write workload
  PROCEDURE oltp_read_write (
    v_id      IN  PLS_INTEGER,
    v_n       IN  PLS_INTEGER,
    v_m       IN  PLS_INTEGER,
    v_c       IN  CHAR,
    v_p       IN  VARCHAR2,
    errCode   OUT PLS_INTEGER,
    errText   OUT VARCHAR2) IS

    -- Store the result of the column 'c'
    cValue  char(120);

    -- Store the sum of the rows in (n..m)
    sumK    number(38,0);

  BEGIN

    errCode := 0;
    errtext := 'OK';

    -- oltp point query
    FOR i in 1 .. 10 LOOP

      -- DBMS_OUTPUT.PUT_LINE('oltp point query');
      SELECT c INTO cValue FROM sbtest WHERE id = v_id;
      -- DBMS_OUTPUT.PUT_LINE('c = ' || cValue);

    END LOOP;

    -- oltp range query (using a cursor for loop)
--    DBMS_OUTPUT.PUT_LINE('oltp range query');
    FOR range_rows IN range_query(v_n, v_m)
    LOOP
--      DBMS_OUTPUT.PUT_LINE(range_query%ROWCOUNT || ' c = ' || range_rows.c);
      null;
    END LOOP;

    -- olpt range SUM() query
--    DBMS_OUTPUT.PUT_LINE('oltp range SUM() query');
    SELECT sum(k) INTO sumK FROM sbtest WHERE id BETWEEN v_n AND v_m;
--    DBMS_OUTPUT.PUT_LINE('sumK = ' || sumK);

    -- oltp range ORDER BY query (using explict fetches)
--    DBMS_OUTPUT.PUT_LINE('oltp range ORDER BY query');
    OPEN range_order_query(v_n, v_m);
    LOOP
      FETCH range_order_query INTO cValue;
      EXIT WHEN range_order_query%NOTFOUND;
--      DBMS_OUTPUT.PUT_LINE('c = ' || cValue);
    END LOOP;
    CLOSE range_order_query;

    -- oltp range DISTINCT query
--    DBMS_OUTPUT.PUT_LINE('oltp range DISTINCT query');
    OPEN range_distinct_query(v_n, v_m);
    LOOP
      FETCH range_distinct_query INTO cValue;
      EXIT WHEN range_distinct_query%NOTFOUND;
--      DBMS_OUTPUT.PUT_LINE('c = ' || cValue);
    END LOOP;
    CLOSE range_distinct_query;

    -- oltp UPDATES on index column
--    DBMS_OUTPUT.PUT_LINE('oltp UPDATES on index column');
    UPDATE sbtest 
      SET k = k + 1 
      WHERE id = v_n;

    -- oltp UPDATES on non-index column
--    DBMS_OUTPUT.PUT_LINE('oltp UPDATES on non-index column');
    UPDATE sbtest 
      SET c =  v_n
      WHERE id = v_m; 

    -- oltp DELETE query
--    DBMS_OUTPUT.PUT_LINE('oltp DELETE query');
    DELETE FROM sbtest 
      WHERE id = v_n;

    -- oltp INSERT query
--    DBMS_OUTPUT.PUT_LINE('oltp INSERT query');
    INSERT INTO sbtest (id, k, c, pad)
      VALUES (v_n, v_m, v_c, v_p);  

    -- Commit the changes
    COMMIT;

  EXCEPTION

    WHEN NO_DATA_FOUND THEN

      errCode  := 0;
      errText  := 'OK';

    WHEN OTHERS THEN

      errCode  := SQLCODE;
      errText  := SUBSTR(SQLERRM, 1, 200);

  END oltp_read_write;

BEGIN  -- package initialization goes here

  -- Run the procedures once to initialize everything
  oltp_read_only(1, 1, 10, theErrCode, theErrText );
  oltp_read_write(1, 1, 10, 'abc', 'def', theErrCode, theErrText );

  DBMS_OUTPUT.PUT_LINE('Initialized the workload package');
END workload;
/
```

`4_call_workload.sql:`

```plsql
set serveroutput on;

declare
  counter    PLS_INTEGER;
  errCode    PLS_INTEGER;
  errtext    VARCHAR2(256);
  line1      VARCHAR2(256);
  line2      VARCHAR2(256);
  someText   sbtest.c%TYPE;
  moreText   VARCHAR2(256);
  i          PLS_INTEGER;
  iterations PLS_INTEGER;
  startTime  NUMBER;
  endTime    NUMBER;
  duration   NUMBER;
begin

  -- Initialize the someText string
  line1 := 'The quick brown foxy did da jumping thing over that lazy doggy. ';
  line2 := 'Question three, who was scott and who or what was tiger?';
  someText := line1 || line2;
  moreText := '';
 
  -- Initialize the moreText string
  FOR i in 1 .. 60 LOOP
    moreText := moreText || 'a';
  END LOOP;
  
  -- Get the start time in centi-seconds
  startTime := DBMS_UTILITY.GET_TIME();

  iterations := 10000;
  for counter in 1 .. iterations LOOP
    workload.oltp_read_only(1, 1, 1100, errCode, errtext);
    if errCode != 0 then
      exit;
    end if;
  end loop;

  -- Get the end time in centi-seconds
  endTime := DBMS_UTILITY.GET_TIME();
  if errCode !=0 then 
    dbms_output.put_line('  ');
    dbms_output.put_line('Error code = ' || errCode || ' Error Text = ' || errtext);
  end if;
  duration := endTime - startTime;
  IF duration > 0 THEN
    dbms_output.put_line('  ');
    dbms_output.put_line('Called workload.oltp_read_only  ' || iterations || ' times. TPS = ' || trunc(iterations / duration * 100, 1) );
  ELSE
    dbms_output.put_line('Could not get valid timing info');
  END IF;


  iterations := 10000;
  for counter in 1 .. iterations LOOP
    workload.oltp_read_write(1, 1, 1100, someText, moreText, errCode, errtext);
    if errCode != 0 then
      exit;
    end if;
  end loop;

  -- Get the end time in centi-seconds
  endTime := DBMS_UTILITY.GET_TIME();

  if errCode !=0 then 
    dbms_output.put_line('  ');
    dbms_output.put_line('Error code = ' || errCode || ' Error Text = ' || errtext);
  end if;

  duration := endTime - startTime;
  IF duration > 0 THEN
    dbms_output.put_line('Called workload.oltp_read_write ' || iterations || ' times. TPS = ' || trunc(iterations / duration * 100, 1) );
  ELSE
    dbms_output.put_line('Could not get valid timing info');
  END IF;
end;
/
```

