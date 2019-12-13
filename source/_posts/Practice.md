---
title: 内存数据库 - Best Practice
date: 2018-11-24 14:18:44
tags:
    - TimesTen
    - 内存数据库
categories:
    - 数据库
---

> Oracle专家的三次授课。

<!--more-->

# Lesson 1

## 创建用户并分配权限

### 创建测试schema，命名为test

```plsql
create user test identified by test;
```

### 分配连接资源

```plsql
grant connect,resource to test;
grant execute on dbms_lock to test;
grant execute on UTL_FILE to test;
```

### 为test用户创建external_data目录以及分配权限

```plsql
create directory external_data as '/home/oracle/data';
grant read,write on directory external_data to test;
```

> 要注意oracle用户必须拥有对这里的external_data路径读写的权限。

### 分配表空间权限

我们知道oracle中没有库的概念，取而代之的是表空间（Tablespace），在oracle初次被安装时，数据库中只有系统本身内置的表空间：

- **SYSTEM** - 存储数据字典
- **SYSAUX** - 存储辅助应用程序的数据
- **TEMP** - 存储数据库临时对象
- **USERS** - 存储各个用户创建的对象
- **UNDOTBS** - 存储不一致数据，用于事物回滚、数据库恢复、读一致性、闪回查询
- ……

而当第一次通过管理员创建一个用户且未为其创建并指定表空间时，数据库系统会为其指定默认的表空间为SYSTEM，而他并没有使用SYSTEM表空间的权限，因此该用户无法完成建表等操作，可通过执行以下操作：

```plsql
-- DBA下执行：
-- 查看数据库中的所有表空间
select * from v$tablespace;
-- 查看当前用户所在的表空间(注意oracle系统表中存储的用户名字段都是大写，要注意这与“oracle中不区分大小写”这一概念区分开来)
select username,default_tablespace from dba_users where username='TEST';
-- 为用户赋予当前表空间下的权限
alter user test quota unlimited on users;
--  或者制定用户可用大小：
alter user test quota 50M on users;
```



## 连接用户，建表，跑存储过程和函数

### 连接test用户

```shell
-- 在系统命令下连接
cd $ORACLE_HOME/bin
./sqlplus test/test
```

```plsql
-- 在进入sqlplus后的连接
conn test/test
```

### 创建表

```plsql
create table t_mobiles(f_id number(6),f_mobile_head varchar2(50),f_province varchar2(50),f_city varchar2(50),f_platform varchar2(50),f_tel_head varchar2(50),f_zipcode varchar2(50),primary key(f_id));
COMMENT ON COLUMN T_MOBILES.F_ID IS '主键';
COMMENT ON COLUMN T_MOBILES.F_MOBILE_HEAD IS '手机号段';
COMMENT ON COLUMN T_MOBILES.F_PROVINCE IS '省份地区';
COMMENT ON COLUMN T_MOBILES.F_CITY IS '城市';
COMMENT ON COLUMN T_MOBILES.F_PLATFORM IS '运营商';
COMMENT ON COLUMN T_MOBILES.F_TEL_HEAD IS '固话区号';
COMMENT ON COLUMN T_MOBILES.F_ZIPCODE IS '邮政编码';
COMMENT ON TABLE T_MOBILES  IS '号段表';

create table t_records(f_id number(6),f_no varchar2(50),f_begin_time date,f_end_time date,f_duration number(10,0),f_province VARCHAR2(50), f_platform varchar2(50), f_mobile NUMBER(1) DEFAULT -1);
--*注：因f_id导入时缺少数据，所有先不设置为PK.
COMMENT ON COLUMN T_RECORDS.F_ID IS '主键';
COMMENT ON COLUMN T_RECORDS.F_NO IS '通话号码';
COMMENT ON COLUMN T_RECORDS.F_BEGIN_TIME IS '开始时间';
COMMENT ON COLUMN T_RECORDS.F_END_TIME IS '结束时间';
COMMENT ON COLUMN T_RECORDS.F_DURATION IS '通话时长';
COMMENT ON COLUMN T_RECORDS.F_PROVINCE IS '省份地区';
COMMENT ON COLUMN T_RECORDS.F_PLATFORM IS '运营商';
COMMENT ON COLUMN T_RECORDS.F_MOBILE IS '手机号码标志';
COMMENT ON TABLE T_RECORDS  IS '通话清单表';
```

### 创建ctl文件导入csv数据

进入`external_data`路径下并创建以下文件：

```shell
$ cd /home/oracle/data
$ vi control_mobiles.ctl
$ vi control_records.ctl
```

`control_mobiles.ctl:`

```ctl
LOAD DATA
CHARACTERSET UTF8
INFILE '/home/oracle/data/mobiles.csv'
TRUNCATE INTO TABLE t_mobiles
FIELDS TERMINATED BY ',' OPTIONALLY ENCLOSED BY '"'
TRAILING NULLCOLS
(
	F_ID,
	F_MOBILE_HEAD,
	F_PROVINCE,
	F_CITY,
	F_PLATFORM,
	F_TEL_HEAD,
	F_ZIPCODE
)
```

`control_records.ctl:`

```
LOAD DATA
CHARACTERSET UTF8
INFILE '/home/oracle/data/records.csv'
TRUNCATE INTO TABLE t_records
FIELDS TERMINATED BY ',' OPTIONALLY ENCLOSED BY '"'
TRAILING NULLCOLS
(
	F_NO,
	F_BEGIN_TIME DATE "YYYY-MM-DD HH24:MI:SS",
	F_END_TIME DATE "YYYY-MM-DD HH24:MI:SS",
	F_DURATION INTEGER EXTERNAL
)
```

在该路径下执行导入操作：

```shell
$ $ORACLE_HOME/bin/sqlldr userid=test/test control=control_mobiles.ctl
$ $ORACLE_HOME/bin/sqlldr userid=test/test control=control_records.ctl
```

> 教程中命令为：
>
> ```shell
> $ sqlldr userid=test/test@orcl control=control_mobiles.ctl
> ```
>
> 即在导入时指定`连接字符串`（这里的orcl实际上是连接字符串的别名），其在`$ORACLE_HOME/network/admin/tnsname.ora`中被声明，但是默认状态下oracle中并没有配置该连接字符串，意味着我们在连接时不需要为其指定值。
>
> 既然如此，应用程序该如何在未进行上述配置的情况下连接到该字符串呢？这里就是`连接字符串`和`服务名`的区别，oracle有个默认服务名`XE`，实际上oracle中还有多个备用服务，当XE服务崩掉的时候会自动切换到备用服务。连接字符串如下：
>
> ```
> jdbc:oracle:thin:@localhost:1521:XE
> ```
>
> 那么没有配置连接字符串别名时，sqlplus如何通过此方法连接呢？如下直接将连接字符串全部写全：
>
> ```shell
> # 命令格式：sqlplus username/password@host:port/service_name
> $ sqlplus tanrui/tanrui@127.0.0.1:1521/xe
> ```
>



## 数据预处理

```plsql
-- 1、创建序列seq_records_pk用于生成通话记录表t_records的主键
create sequence seq_records increment by 1 start with 1 ;

-- 2、修补通话记录表t_records的主键数据，并把f_id改为主键
update t_records set f_id=seq_records.nextval;
alter table t_records add constraint t_records_pk primary key (f_id);

-- 3、创建并初始化同步锁表，用于多线程同步控制
CREATE TABLE T_LOCK(F_NAME VARCHAR2(30),F_INDEX NUMBER(20,0),PRIMARY KEY(F_NAME));
COMMENT ON COLUMN T_LOCK.F_NAME IS '锁名';
COMMENT ON COLUMN T_LOCK.F_INDEX IS '锁的当前值';
COMMENT ON TABLE T_LOCK  IS '同步锁表';
insert into T_LOCK values('_RECORD_INDEX',0);

-- 4、在电话号段表中创建唯一性索引，提高号段检索速度
create unique index uniq_mobile_head on t_mobiles(f_mobile_head);
update t_mobiles set f_province = '内蒙古' where f_province = '内蒙';

-- 5、创建日志表，用于记录程序执行过程中的日志信息。
create table t_log(f_time date, f_head varchar2(20), f_content varchar2(500));
COMMENT ON COLUMN T_LOG.F_TIME IS '日志时间';
COMMENT ON COLUMN T_LOG.F_HEAD IS '日志类型标志';
COMMENT ON COLUMN T_LOG.F_CONTENT IS '日志内容';
COMMENT ON TABLE T_LOG  IS '日志表';
```



## 创建函数和存储过程

### 声明函数和存储过程

- 函数is_mobile，判断通话号码是否为手机号码

```plsql
--函数：判断通话号码是否为手机号码
CREATE OR REPLACE FUNCTION is_mobile(phone VARCHAR2)
    RETURN BOOLEAN IS

    v_phone VARCHAR2(20);
    v_head VARCHAR2(2);
BEGIN
    --检查参数func
    IF phone IS NULL THEN
        RETURN FALSE;
    END IF;

	--去除前后空格
    v_phone := TRIM(phone);

	--去除号码前面的0
    IF substr(v_phone,0,1) = '0' THEN
        v_phone := substr(v_phone, 2);
    END IF;

	--检查手机号码长度
    IF substr(v_phone,0,1) <> '1' OR LENGTH(v_phone) <> 11 THEN
        RETURN FALSE;
    END IF;

	--截取号码前两位
    v_head := substr(v_phone,1,2);

    IF v_head = '13' OR v_head = '14' OR v_head ='15' OR v_head ='17' OR v_head = '18' THEN
        RETURN TRUE;
    ELSE
        RETURN FALSE;
    END IF;
END;
/
```

- 存储过程init，清空t_log，同时t_lock置零

```plsql
--存储过程：初始化测试数据
CREATE OR REPLACE PROCEDURE init IS
    CURSOR job_cursor IS SELECT JOB FROM user_jobs;
BEGIN
	--重置处理位置为0
    EXECUTE IMMEDIATE 'update t_lock set f_index=0';

	--清除日志表中的记录
    EXECUTE IMMEDIATE 'truncate table t_log';

	--重置话单表中的记录
    EXECUTE IMMEDIATE 'update t_records set f_province = NULL,f_platform=NULL, f_mobile=-1';
    COMMIT;

    FOR tmp_job IN job_cursor LOOP
        dbms_job.broken(tmp_job.JOB,TRUE,sysdate);
        dbms_job.REMOVE(tmp_job.JOB);
    END LOOP;
END;
/
```

- 存储过程print，打印日志，存到T_LOG表中

```plsql
--存储过程：打印日志
CREATE OR REPLACE PROCEDURE print(prefix VARCHAR2, content VARCHAR2) IS
BEGIN
	--dbms_output.put_line(to_char('yyyy-mm-dd hh24:mi:ss')||','||prefix||','||content);
	INSERT INTO t_log VALUES(sysdate,prefix, content);
	COMMIT;
EXCEPTION
	WHEN OTHERS THEN
		dbms_output.put_line('Error code: '||SQLCODE);
		dbms_output.put_line('Error mesg: '||sqlerrm);
END;
/
```

- 存储过程show，显示当前处理情况

```plsql
--存储过程：显示当前处理情况
CREATE OR REPLACE PROCEDURE show IS
	--待处理记录总数
    v_record_count NUMBER;

	--当前日志表记录总数
    v_log_count NUMBER;

	--当前数据处理位置
    v_current_index NUMBER;

	--用户Job表游标
    CURSOR job_cursor IS SELECT * FROM user_jobs;

BEGIN
    SELECT COUNT(1) INTO v_log_count FROM t_log;
    SELECT f_index INTO v_current_index FROM t_lock;
    SELECT COUNT(1) INTO v_record_count FROM t_records;

	dbms_output.put_line('log count: '||v_log_count);
    dbms_output.put_line('record count: '||v_record_count);
    dbms_output.put_line('current index: '||v_current_index);

	--清除用户job记录
    FOR tmp_job IN job_cursor LOOP
        dbms_output.put_line('job:'||tmp_job.JOB||',broken:'||tmp_job.broken||',total_time:'||tmp_job.total_time||',failures:'||tmp_job.failures||',interval:'||tmp_job.INTERVAL||',last_sec:'||tmp_job.last_sec||',next_sec:'||tmp_job.next_sec);

    END LOOP;
END;
/
```

- 存储过程process_data，提交一个job处理数据

> **共享锁和排它锁:**
>
> - 当某事务对数据添加共享锁时，此时该事务`只能读不能写`，其他事务只能对该数据添加共享锁，而不能添加排它锁
>
> - 当某事务对数据添加排它锁时，此时该事务`既能读又能写`，其他事务不能对该数据添加任何锁
>
> **autocommit需要关掉:**
>
>
> 假设现在有三个job对T_LOCK表进行并发读写，如下：
>
> ![image-20181124202306595](/images/image-20181124202306595.png)
>
> 步骤如下：
>
> ![锁](/images/锁.png)
>
> 阻塞情况：
>
> ![锁2](/images/锁2.png)

```plsql
--存储过程：处理数据
CREATE OR REPLACE PROCEDURE process_data(process_no IN NUMBER, batch_size IN NUMBER) IS
    --定义常量
    c_record_index CONSTANT VARCHAR2(20)   :='_RECORD_INDEX';
    c_process_prefix CONSTANT VARCHAR2(20) := '[  PROCESS ]';
    c_select_record_sql VARCHAR2(100)  := 'select * from t_records where f_id >= :x and f_id <= :y';
    c_select_mobile_sql VARCHAR2(100)  := 'select * from t_mobiles where f_mobile_head = :x';
    c_update_mobile_sql VARCHAR2(100)  := 'update t_records set f_province = :x, f_platform = :y, f_mobile = 1 where f_id = :z';
    c_update_record_sql VARCHAR2(100)  := 'update t_records set f_mobile = 0 where f_id = :n';
    v_record_count NUMBER;
    v_current_index NUMBER;
    v_begin_index NUMBER;
    v_end_index NUMBER;
    v_id NUMBER;
    v_phone VARCHAR2(20);
    v_province VARCHAR2(20);
    v_platform VARCHAR2(20);
    --定义动态游标
    TYPE ty_record_cursor IS REF CURSOR;
    record_cursor ty_record_cursor;
    mobile_cursor ty_record_cursor;
    v_record_row t_records%rowtype;
    v_mobile_row t_mobiles%rowtype;
BEGIN
    PRINT(c_process_prefix, 'process['||process_no||'], running...');
    --获取待处理的记录总数
    SELECT COUNT(1) INTO v_record_count FROM t_records;
    PRINT(c_process_prefix, 'process['||process_no||'], records count: '||v_record_count);
    LOOP
        --获取记录锁
        SELECT f_index INTO v_current_index FROM t_lock WHERE f_name = c_record_index FOR UPDATE;
        PRINT(c_process_prefix, 'process['||process_no||'], current index: '||v_current_index);
        IF v_current_index = v_record_count THEN
            PRINT(c_process_prefix, 'process['||process_no||'], finished.');
            EXIT;
        END IF;
        --记录本次处理的开始和结束记录位置
        v_end_index := v_current_index + batch_size;
        IF v_end_index > v_record_count THEN
            v_end_index := v_record_count;
        END IF;
        --提交事务，释放锁
        UPDATE t_lock SET f_index = v_end_index WHERE f_name =c_record_index;
        COMMIT;
        --计算开始位置
        v_begin_index := v_current_index +1;
        PRINT(c_process_prefix, 'process['||process_no||'], begin index:'||v_begin_index||', end index:'||v_end_index);
        --test：dbms_lock.sleep(5);
        --查询一批记录进行逐个处理
        OPEN record_cursor FOR c_select_record_sql USING v_begin_index, v_end_index;
        LOOP
            FETCH record_cursor INTO v_record_row;
            EXIT WHEN record_cursor%notfound;
            v_id    := v_record_row.f_id;
            v_phone := v_record_row.f_no;
            IF is_mobile(v_phone) THEN
                v_phone := TRIM(v_phone);
                IF substr(v_phone,0,1) = '0' THEN
                    v_phone := substr(v_phone, 2);
                END IF;
                --PRINT(c_process_prefix, 'process['||process_no||'], id:'||v_id||', phone:'||v_phone);
                --更新话单记录中的省份、运营商以及手机类型标志
                OPEN mobile_cursor FOR c_select_mobile_sql USING substr(v_phone,1,7);
                    FETCH mobile_cursor INTO v_mobile_row;
                    v_province := v_mobile_row.f_province;
                    v_platform := v_mobile_row.f_platform;
                    --FETCH mobile_cursor INTO v_province, v_platform;
                CLOSE mobile_cursor;
				--更新话单记录的运营商、省份地区信息
                EXECUTE IMMEDIATE c_update_mobile_sql USING v_province,v_platform,v_id;
            ELSE
                --更新话单记录为非移动号码类型
                EXECUTE IMMEDIATE c_update_record_sql USING v_id;
            END IF;
			--提交事务
            COMMIT;
        END LOOP;
        CLOSE record_cursor;
        PRINT(c_process_prefix, 'process['||process_no||'], processed index: '||v_end_index);
    END LOOP;
EXCEPTION
    WHEN OTHERS THEN
        dbms_output.put_line('Error code: '||SQLCODE);
        dbms_output.put_line('Error mesg: '||sqlerrm);
END;
/
```

- 存储过程generate_csv_report，生成报表

```plsql
--存储过程：生成报表
CREATE OR REPLACE PROCEDURE generate_csv_report IS
	c_report_prefix CONSTANT VARCHAR2(20) := '[  REPORT  ]';
    v_report_1  UTL_FILE.FILE_TYPE;
    v_report_2  UTL_FILE.FILE_TYPE;
    CURSOR report_1_cursor IS SELECT f_platform,f_province,SUM(f_duration) total FROM t_records WHERE f_mobile=1 GROUP BY f_platform,f_province ORDER BY f_platform ASC,SUM(f_duration) DESC;
    cursor report_2_cursor is select f_province,f_platform,sum(f_duration) total from t_records where f_mobile=1 group by f_province,f_platform order by f_province asc,sum(f_duration) desc;
BEGIN
    --生成报表1，根据运营商分类汇总各省份地区的通话量
    v_report_1 := UTL_FILE.FOPEN( LOCATION => 'EXTERNAL_DATA', filename => 'report1.csv', open_mode => 'w', max_linesize => 32767);
    FOR cur_tmp IN report_1_cursor LOOP
        UTL_FILE.PUT_LINE(v_report_1, cur_tmp.f_platform || ',' || cur_tmp.f_province || ',' || cur_tmp.total);
    END LOOP;
    UTL_FILE.FCLOSE(v_report_1);
    --生成报表2，根据各省份地区汇总各运营商的通话量
    v_report_2 := UTL_FILE.FOPEN( LOCATION => 'EXTERNAL_DATA', filename => 'report2.csv', open_mode => 'w', max_linesize => 32767);
    FOR cur_tmp IN report_2_cursor LOOP
        UTL_FILE.PUT_LINE(v_report_2, cur_tmp.f_province || ',' || cur_tmp.f_platform || ',' ||  cur_tmp.total);
    END LOOP;
    UTL_FILE.FCLOSE(v_report_2);
    PRINT(c_report_prefix, 'generated reports.');
    EXCEPTION
        WHEN OTHERS THEN
            dbms_output.put_line('Error code: '||SQLCODE);
            dbms_output.put_line('Error mesg: '||sqlerrm);
END;
/
```

- 存储过程analysis，调用上述函数，完成任务逻辑，支持指定任务个数和一批数量

> **[dbms_job](https://docs.oracle.com/cd/B28359_01/appdev.111/b28419/d_job.htm#BABHCBFD):**
>
> 用于管理job的package
>
> **oracle限定的job_queue_processes:**
>
> oracle中有一个对任务可启动进程的数量进行限制的参数：
>
> ```plsql
> SQL> show parameter job_queue_processes;
> NAME				     TYPE	 VALUE
> ----------------------------------------------------------
> job_queue_processeses		 integer	 10
>
> SQL> alter system set job_queue_processes=0...1000;
> ```
>
> **使用ctrl+c是无法停止job的:**
>
> 可使用`top`命令查看当前进程详情，如果需要结束特定job可kill对应job的进程号

```plsql
CREATE OR REPLACE PROCEDURE analysis (job_count IN NUMBER, batch_size IN NUMBER)IS
    --定义常量
    c_record_index CONSTANT VARCHAR2(20)	:='_RECORD_INDEX';
	  c_analysis_prefix CONSTANT VARCHAR2(20)	:= '[ ANALYSIS ]';
    --当前处理位置
    v_record_index NUMBER;
    --待处理的记录总数
    v_record_count NUMBER;
    --保存临时创建的job no
    v_tmp_jobno NUMBER;
    --开始结束时间
    v_begin_time NUMBER;
    v_process_end_time NUMBER;
    v_analysis_end_time NUMBER;
    --异常变量
    e_invalid_input EXCEPTION;
BEGIN
    PRINT(c_analysis_prefix, ' start analysis...');
    --输入参数检查
    IF job_count < 1 OR batch_size<1 THEN
        RAISE e_invalid_input;
    END IF;
    PRINT(c_analysis_prefix, ' checked input parameters.');
    --记录开始时间
    v_begin_time := dbms_utility.get_time;
    --获取待处理的记录总数
    SELECT COUNT(1) INTO v_record_count FROM t_records;
    PRINT(c_analysis_prefix, ' records count: '||v_record_count);
    --开始计算重置为0
    UPDATE t_lock SET f_index=0 WHERE f_name=c_record_index;
    COMMIT;
    PRINT(c_analysis_prefix, ' reset index to zero.');
    --提交多个job
    FOR I IN 1.. job_count LOOP
        dbms_job.submit(v_tmp_jobno,'begin process_data('||I||','||batch_size||'); end;');
        PRINT(c_analysis_prefix, ' submitted new job, no: '||v_tmp_jobno);
    END LOOP;
    PRINT(c_analysis_prefix, ' created '||job_count||' jobs.');
    --定时检查处理进度
    LOOP
        SELECT f_index INTO v_record_index FROM t_lock WHERE f_name = c_record_index;
        PRINT(c_analysis_prefix, ' current index: '||v_record_index);
        IF v_record_index = v_record_count THEN
            PRINT(c_analysis_prefix, ' processed all records, exiting...');
            EXIT;
        ELSE
            dbms_lock.sleep(5);--暂停等待5秒
        END IF;
    END LOOP;
    v_process_end_time := dbms_utility.get_time;
    PRINT(c_analysis_prefix, 'process, elapsed time: '||(v_process_end_time-v_begin_time)/100||' seconds.');
    dbms_output.put_line('process, elapsed time: '||(v_process_end_time-v_begin_time)/100||' seconds.');
    --分类汇总产生报表
    generate_csv_report;
    --结束时间
    v_analysis_end_time := dbms_utility.get_time;
    PRINT(c_analysis_prefix, 'report, elapsed time: '||(v_analysis_end_time-v_process_end_time)/100||' seconds.');
    dbms_output.put_line('report, elapsed time: '||(v_analysis_end_time-v_process_end_time)/100||' seconds.');
--异常捕获部分
EXCEPTION
    WHEN e_invalid_input THEN
        dbms_output.put_line('Invalid input values, job_count:'||job_count||', batch_size:'||batch_size);
    WHEN OTHERS THEN
        dbms_output.put_line('Error code: '||SQLCODE);
        dbms_output.put_line('Error mesg: '||sqlerrm);
END;
/
```

- 存储过程mul_analysis，循环调用analysis，指定不同的任务个数和批数量，并将运行时间存入T_RESULT中

```plsql
-- 调用多次analysis，指定不同的job数和批数
create or replace procedure mul_analysis is
    -- 最小job数
    v_begin_job_no NUMBER := 3;
    -- 最大job数
    v_end_job_no NUMBER := 8;
    -- 每次增长的batch数量
    v_range NUMBER := 2000;
    -- 最小batch数量
    v_begin_range NUMBER := 1000;
    -- 最大batch数量
    v_end_range NUMBER := 10000;
    -- 当前range
    range NUMBER;
    begin
        for I in v_begin_job_no..v_end_job_no LOOP
            range := v_begin_range;
            LOOP
                -- 清洗表
                init();
                -- 分析
                analysis(I, range);
                range := range+v_range;
				-- range增长到10000则停止
                if range > v_end_range then
                    exit;
                end if;
            end loop;
        end loop;
    end;
/
```

### 执行函数和存储过程

> 在sqlplus中执行函数和存储过程之前需先打开serveroutput，即：
>
> ```plsql
> SQL> set serveroutput on;
> ```
>
> 这是因为存储过程中用到了`dbms_output.put_line`，上述语句是相当于告诉pl/sql引擎将`dbms_output.put_line`传递到缓冲区的内容输出到主控制台上。

```
call init();
call analysis(4,1000);
```



## 结果分析

通过执行`mul_analysis()`对一系列job和batch组合值进行测试，结果如下：

![image-20181124205409162](/images/image-20181124205409162.png)


# Lesson 2

## 创建用户并分配权限

### 创建用户

```plsql
create user audittest identified by audittest;
```

### 分配权限

```plsql
grant connect,resource to audittest;
grant execute on dbms_lock to audittest;
alter user audittest quota unlimited on users;
conn audittest/audittest;
```

## 创建表及其他对象

### 创建表

> 注意：
>
> 这里在创建表时添加了`ENABLE`限制条件，oracle中对表和列的约束有`Enable`/`Disable`(启用/禁用)和`Validate`/`NoValidate`(验证/不验证)
>
> 举两个例子：
>
> **需更改的错误：**
>
> ```plsql
> -- 创建表，对name字段添加唯一性约束
> drop table T_TEST;
> create table T_TEST(
>   id int primary key ,
>   name varchar2(10) constraint unique_name unique disable
> );
> -- 由于某些错误，添加的记录违反了唯一性约束，但添加不会报错
> begin
>   insert into T_TEST values (1, 'tan');
>   insert into T_TEST values (2, 'rui');
>   insert into T_TEST values (3, 'tan');
> end;
> select * from T_TEST;
> -- 修改掉违反唯一性约束的值
> update T_TEST set name='chen' where id=3;
> -- 使得唯一性约束生效
> alter table T_TEST modify constraint unique_name enable;
> select * from T_TEST;
>
> ```
>
> **需保留的错误：**
>
> ```plsql
> -- 创建表，无约束
> drop table T_TEST;
> create table T_TEST(
>   id int primary key ,
>   name varchar2(10)
> );
> -- 一些old的记录本身可能存在重复数据
> begin
>   insert into T_TEST values (1, 'tan');
>   insert into T_TEST values (2, 'rui');
>   insert into T_TEST values (3, 'tan');
> end;
> select * from T_TEST;
> -- 对name列创建非唯一性索引
> create index i_name on T_TEST(name);
> -- 新要求需要对name添加唯一性约束unique_name，但保留旧值，注意这里一定要使用非唯一性索引
> alter table T_TEST add constraint unique_name unique(name) using index i_name ENABLE NOVALIDATE ;
> -- 此时无法插入name相同的数据了
> insert into T_TEST values (4, 'tan');
> ```

```plsql
--部门表
CREATE TABLE "AUDITTEST"."T_DEPARTMENT"
(	"F_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_NAME" VARCHAR2(50 BYTE) NOT NULL ENABLE,
	"F_CODE" VARCHAR2(50 BYTE) NOT NULL ENABLE,
	"F_PARENT_ID" NUMBER(6,0),
	"F_MANAGER" VARCHAR2(50 BYTE) NOT NULL ENABLE,
	"F_REMARK" VARCHAR2(200 BYTE),
	 CONSTRAINT "T_DEPARTMENT_PK" PRIMARY KEY ("F_ID")
) ;

COMMENT ON COLUMN "AUDITTEST"."T_DEPARTMENT"."F_ID" IS 'PK';
COMMENT ON COLUMN "AUDITTEST"."T_DEPARTMENT"."F_NAME" IS '部门名称';
COMMENT ON COLUMN "AUDITTEST"."T_DEPARTMENT"."F_CODE" IS '部门编号';
COMMENT ON COLUMN "AUDITTEST"."T_DEPARTMENT"."F_PARENT_ID" IS '上级部门ID';
COMMENT ON COLUMN "AUDITTEST"."T_DEPARTMENT"."F_MANAGER" IS '部门经理';
COMMENT ON COLUMN "AUDITTEST"."T_DEPARTMENT"."F_REMARK" IS '备注信息';
COMMENT ON TABLE "AUDITTEST"."T_DEPARTMENT"  IS '部门表';

--用户表
CREATE TABLE "AUDITTEST"."T_USER"
(	"F_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_DEPT_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_NAME" VARCHAR2(50 BYTE) NOT NULL ENABLE,
	"F_CODE" VARCHAR2(20 BYTE),
	"F_SEX" VARCHAR2(5 BYTE) DEFAULT NULL,
	"F_MOBILE" VARCHAR2(20 BYTE),
	"F_TELEPHONE" VARCHAR2(20 BYTE),
	"F_EMAIL" VARCHAR2(50 BYTE),
	"F_REMARK" VARCHAR2(200 BYTE),
	 CONSTRAINT "T_USER_PK" PRIMARY KEY ("F_ID")
);

COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_ID" IS 'PK';
COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_DEPT_ID" IS '部门ID';
COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_NAME" IS '用户名';
COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_CODE" IS '员工编号';
COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_SEX" IS '性别';
COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_MOBILE" IS '手机';
COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_TELEPHONE" IS '固话';
COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_EMAIL" IS '邮箱';
COMMENT ON COLUMN "AUDITTEST"."T_USER"."F_REMARK" IS '备注信息';
COMMENT ON TABLE "AUDITTEST"."T_USER"  IS '用户表';

--客户信息表
CREATE TABLE "AUDITTEST"."T_CUSTOMER"
(	"F_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_CODE" VARCHAR2(45 BYTE) NOT NULL ENABLE,
	"F_FULL_NAME" VARCHAR2(145 BYTE) NOT NULL ENABLE,
	"F_LINKMAN" VARCHAR2(45 BYTE) NOT NULL ENABLE,
	"F_MOBILE" VARCHAR2(11 BYTE) NOT NULL ENABLE,
	"F_TELEPHONE" VARCHAR2(20 BYTE),
	"F_EMAIL" VARCHAR2(60 BYTE),
	"F_ADDRESS" VARCHAR2(200 BYTE),
	"F_CITY" VARCHAR2(45 BYTE),
	"F_BALANCE" NUMBER(10,2) NOT NULL ENABLE,
	"F_PARTNER" VARCHAR2(45 BYTE),
	"F_REMARK" VARCHAR2(200 BYTE),
	"F_SALESMAN" VARCHAR2(45 BYTE) NOT NULL ENABLE,
	"F_DELETED_TAG" NUMBER(1,0) DEFAULT 0 NOT NULL ENABLE,
	"F_CREATED_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_CREATED_TIME" TIMESTAMP (6) NOT NULL ENABLE,
	"F_MODIFIED_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_MODIFIED_TIME" TIMESTAMP (6) NOT NULL ENABLE,
	"F_VERSION" NUMBER(6,0) DEFAULT 1 NOT NULL ENABLE,
	 PRIMARY KEY ("F_ID")
);

COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_ID" IS '主键';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_CODE" IS '客户编码';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_FULL_NAME" IS '客户全名';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_LINKMAN" IS '联系人';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_MOBILE" IS '联系手机';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_TELEPHONE" IS '联系固话';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_EMAIL" IS '联系邮箱';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_ADDRESS" IS '地址';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_CITY" IS '城市';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_BALANCE" IS '余额';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_PARTNER" IS '所属合作伙伴';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_REMARK" IS '备注信息';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_SALESMAN" IS '业务员';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_DELETED_TAG" IS '删除标志，0：可用，1：已删除';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_CREATED_ID" IS '创建人';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_CREATED_TIME" IS '创建时间';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_MODIFIED_ID" IS '最后修改人';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_MODIFIED_TIME" IS '最后修改时间';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER"."F_VERSION" IS '版本号';
COMMENT ON TABLE "AUDITTEST"."T_CUSTOMER"  IS '客户信息表';

--
CREATE TABLE "AUDITTEST"."T_CUSTOMER_HISTORY"
(	"F_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_CODE" VARCHAR2(45 BYTE) NOT NULL ENABLE,
	"F_FULL_NAME" VARCHAR2(145 BYTE) NOT NULL ENABLE,
	"F_LINKMAN" VARCHAR2(45 BYTE) NOT NULL ENABLE,
	"F_MOBILE" VARCHAR2(11 BYTE) NOT NULL ENABLE,
	"F_TELEPHONE" VARCHAR2(20 BYTE),
	"F_EMAIL" VARCHAR2(60 BYTE),
	"F_ADDRESS" VARCHAR2(200 BYTE),
	"F_CITY" VARCHAR2(45 BYTE),
	"F_BALANCE" NUMBER(10,2) NOT NULL ENABLE,
	"F_PARTNER" VARCHAR2(45 BYTE),
	"F_REMARK" VARCHAR2(200 BYTE),
	"F_SALESMAN" VARCHAR2(45 BYTE) NOT NULL ENABLE,
	"F_DELETED_TAG" NUMBER(1,0) DEFAULT 0 NOT NULL ENABLE,
	"F_CREATED_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_CREATED_TIME" TIMESTAMP (6) NOT NULL ENABLE,
	"F_MODIFIED_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_MODIFIED_TIME" TIMESTAMP (6) NOT NULL ENABLE,
	"F_VERSION" NUMBER(6,0) DEFAULT 1 NOT NULL ENABLE,
	 CONSTRAINT "T_CUSTOMER_HISTORY_PK" PRIMARY KEY ("F_ID", "F_VERSION")
);

--客户信息历史表
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_ID" IS '主键';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_CODE" IS '客户编码';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_FULL_NAME" IS '客户全名';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_LINKMAN" IS '联系人';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_MOBILE" IS '联系手机';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_TELEPHONE" IS '联系固话';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_EMAIL" IS '联系邮箱';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_ADDRESS" IS '地址';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_CITY" IS '城市';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_BALANCE" IS '余额';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_PARTNER" IS '所属合作伙伴';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_REMARK" IS '备注信息';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_SALESMAN" IS '业务员';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_DELETED_TAG" IS '删除标志，0：可用，1：已删除';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_CREATED_ID" IS '创建人';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_CREATED_TIME" IS '创建时间';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_MODIFIED_ID" IS '最后修改人';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_MODIFIED_TIME" IS '最后修改时间';
COMMENT ON COLUMN "AUDITTEST"."T_CUSTOMER_HISTORY"."F_VERSION" IS '版本号';
COMMENT ON TABLE "AUDITTEST"."T_CUSTOMER_HISTORY"  IS '客户信息历史表';

--审计表
CREATE TABLE "AUDITTEST"."T_AUDIT"
(	"F_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_TABLE_NAME" VARCHAR2(50 BYTE) NOT NULL ENABLE,
	"F_ROW_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_NEW_VERSION" NUMBER(6,0) NOT NULL ENABLE,
	"F_COLUMN_NAME" VARCHAR2(50 BYTE) NOT NULL ENABLE,
	"F_OLD_VALUE" VARCHAR2(200 BYTE),
	"F_NEW_VALUE" VARCHAR2(200 BYTE),
	"F_OPERATOR_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_OPERATION_TIME" TIMESTAMP (6) NOT NULL ENABLE,
	 CONSTRAINT "T_AUDIT_PK" PRIMARY KEY ("F_ID")
);

COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_ID" IS '主键';
COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_TABLE_NAME" IS '表名';
COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_ROW_ID" IS '业务数据主键';
COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_NEW_VERSION" IS '新的版本号';
COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_COLUMN_NAME" IS '字段';
COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_OLD_VALUE" IS '原值';
COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_NEW_VALUE" IS '新值';
COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_OPERATOR_ID" IS '操作用户';
COMMENT ON COLUMN "AUDITTEST"."T_AUDIT"."F_OPERATION_TIME" IS '操作时间';
COMMENT ON TABLE "AUDITTEST"."T_AUDIT"  IS '审计表';
```

### 创建索引、序列

```plsql
-- 创建复合索引
CREATE INDEX "AUDITTEST"."IDX_TABLE_ROWID" ON "AUDITTEST"."T_AUDIT" ("F_TABLE_NAME", "F_ROW_ID") ;
-- 创建序列
CREATE SEQUENCE  SEQ_AUDIT_PK  INCREMENT BY 1 START WITH 1;
```

### 创建触发器

```plsql
--创建触发器
create or replace trigger trg_customer_audit
before update on t_customer
for each row
declare
    c_insert_sql constant varchar2(100) := 'insert into t_audit values(:1,:2,:3,:4,:5,:6,:7,:8,systimestamp)';
    c_table_name constant varchar2(20)  := 'T_CUSTOMER';
    v_column_name varchar2(20);
begin
    --记录当前数据到历史表
    insert into t_customer_history values(:old.f_id,:old.f_code,:old.f_full_name,:old.f_linkman,:old.f_mobile,:old.f_telephone,:old.f_email,:old.f_address,:old.f_city,:old.f_balance,:old.f_partner,:old.f_remark,:old.f_salesman,:old.f_deleted_tag,:old.f_created_id,:old.f_created_time,:old.f_modified_id,:old.f_modified_time,:old.f_version);

    --递增记录的版本号
    :new.f_version := :old.f_version+1;

    --判断字段变化
    if updating('F_LINKMAN') then
        v_column_name := '联系人';
        execute immediate c_insert_sql using seq_audit_pk.nextval,c_table_name,:new.f_id,:old.f_version,v_column_name,:old.f_linkman,:new.f_linkman,:new.f_modified_id;
    end if;

    if updating('F_MOBILE') then
        v_column_name := '手机号码';
        execute immediate c_insert_sql using seq_audit_pk.nextval,c_table_name,:new.f_id,:old.f_version,v_column_name,:old.f_mobile,:new.f_mobile,:new.f_modified_id;
    end if;

    if updating('F_TELEPHONE') then
        v_column_name := '固定电话';
        execute immediate c_insert_sql using seq_audit_pk.nextval,c_table_name,:new.f_id,:old.f_version,v_column_name,:old.f_telephone,:new.f_telephone,:new.f_modified_id;
    end if;

    if updating('F_EMAIL') then
        v_column_name := '电子邮箱';
        execute immediate c_insert_sql using seq_audit_pk.nextval,c_table_name,:new.f_id,:old.f_version,v_column_name,:old.f_email,:new.f_email,:new.f_modified_id;
    end if;

    if updating('F_ADDRESS') then
        v_column_name := '联系地址';
        execute immediate c_insert_sql using seq_audit_pk.nextval,c_table_name,:new.f_id,:old.f_version,v_column_name,:old.f_address,:new.f_address,:new.f_modified_id;
    end if;

    if updating('F_BALANCE') then
        v_column_name := '余额';
        execute immediate c_insert_sql using seq_audit_pk.nextval,c_table_name,:new.f_id,:old.f_version,v_column_name,:old.f_balance,:new.f_balance,:new.f_modified_id;
    end if;
end;
/

--创建过程
--过程：重设序列值
create or replace PROCEDURE reset_seq( seq_name IN VARCHAR2 )
IS
    v_val NUMBER;
BEGIN
    EXECUTE IMMEDIATE 'SELECT ' || seq_name || '.NEXTVAL FROM dual' INTO v_val;

    EXECUTE IMMEDIATE 'ALTER SEQUENCE ' || seq_name || ' INCREMENT BY -' || v_val ||' MINVALUE 0';

    EXECUTE IMMEDIATE 'SELECT ' || seq_name || '.NEXTVAL FROM dual' INTO v_val;

    EXECUTE IMMEDIATE 'ALTER SEQUENCE ' || seq_name || ' INCREMENT BY 1 MINVALUE 0';
end;
/
```

### 创建过程

#### 过程reset_seq

> 将序列为输入参数seq_name的值重置

```plsql
--过程：重设序列值
create or replace PROCEDURE reset_seq( seq_name IN VARCHAR2 )
IS
    v_val NUMBER;
BEGIN
    EXECUTE IMMEDIATE 'SELECT ' || seq_name || '.NEXTVAL FROM dual' INTO v_val;

    EXECUTE IMMEDIATE 'ALTER SEQUENCE ' || seq_name || ' INCREMENT BY -' || v_val ||' MINVALUE 0';

    EXECUTE IMMEDIATE 'SELECT ' || seq_name || '.NEXTVAL FROM dual' INTO v_val;

    EXECUTE IMMEDIATE 'ALTER SEQUENCE ' || seq_name || ' INCREMENT BY 1 MINVALUE 0';
end;
/
```

#### 过程init

> truncate(截断)所有表，重设序列，并添加初始值
>
> 注意：
>
> **`truncate`与`delete`的区别**：delete通常用于删除表中的某些行或者所有行，且delete支持回滚，删除掉的记录的物理空间在commit前并不会被回收。truncate只能删除表的所有行且不支持回滚，删除掉的记录的物理空间也会被立刻回收。
>
> truncate的好处在于当需要删除所有行它比delete要快，尤其在包含大量触发器、索引和其他依赖项的情况下；且它不会改变表结构、表依赖等关系，这一特性又使得它比重建表更有效，删除和重建表会使得表的依赖关系断开，因此需要重新创建依赖、创建约束、赋予权限等等操作。
>
> 但是truncate也有不好的地方，比如说当被truncate的表被依赖时，举例：
>
> ```plsql
> -- 创建表，f_id字段引用T_TEST的主码id
> drop table T_TEST2;
> create table T_TEST2(
>   id1 int primary key ,
>   f_id int,
>   constraint fk
>   foreign key (f_id)
>     references T_TEST(id) on delete cascade
> );
> select *
> from T_TEST2;
> insert into T_TEST2 values(1, 1);
> -- 可级联删除
> delete from T_TEST;
> -- 将外码置为禁用
> alter table T_TEST2 modify constraint fk disable validate ;
> -- 可截断（当不禁用外码时无法截断）
> truncate table T_TEST;
> ```
>
> 可见，可通过禁用约束来完成truncate，但是这些主外键约束应是创建数据库时的我们定义的强制关系，上述方法可能会使得这种强制关系紊乱，因此需做好取舍决策。

```plsql
--过程：数据初始化
create or replace procedure init is
begin
    --清除数据
    execute immediate 'truncate table t_audit';
    execute immediate 'truncate table t_customer_history';
    execute immediate 'truncate table t_customer';
    execute immediate 'truncate table t_user';
    execute immediate 'truncate table t_department';
    --重调序列
    reset_seq('seq_audit_pk');

    --插入部门
    insert into t_department values(1,'销售部','D01',NULL,'李明','备注1...');
    insert into t_department values(2,'销售部-北京分部','D0101',1,'赵军','备注2...');
    insert into t_department values(3,'销售部-上海分部','D0102',1,'张华','备注3...');
    insert into t_department values(4,'销售部-深圳分部','D0103',1,'王兵','备注4...');

    --插入用户
    insert into t_user values(1,1,'仲芳芳','U8201','女','13771234101','02131231011','use1@samtech.com','备注1...');
    insert into t_user values(2,1,'李明申','U8202','男','13771234102','02131231012','use2@samtech.com','备注2...');
    insert into t_user values(3,2,'张雪', 'U8203','女','13771234103','02131231013','use3@samtech.com','备注3...');
    insert into t_user values(4,2,'王刚', 'U8204','男','13771234104','02131231014','use4@samtech.com','备注4...');
    insert into t_user values(5,3,'赵昌日','U8205','男','13771234105','02131231015','use5@samtech.com','备注5...');
    insert into t_user values(6,3,'孙晓华','U8206','男','13771234106','02131231016','use6@samtech.com','备注6...');
    insert into t_user values(7,4,'陈亚男','U8207','女','13771234107','02131231017','use7@samtech.com','备注7...');
    insert into t_user values(8,4,'刘兵超','U8208','男','13771234108','02131231018','use8@samtech.com','备注8...');

    --插入客户
    insert into t_customer
    values(1,'C1808001','上海市永辉电子股份有限公司','张明升','15352678121','02135681589','ming@google.com','上海市静安区城区安泰路1108号','上海市',12082,'上海中远','备注...','张娜',0,1,sysdate,1,sysdate,1);
    commit;
end;
/
```

#### 修改客户信息过程

```plsql
--过程：修改客户地址
create or replace procedure modify_address
(p_row_id in number,p_address in varchar2, p_operator in number)
as
begin
    --校验参数省略
    --...
    execute immediate 'update t_customer set f_address=:1,f_modified_id=:2, f_modified_time=sysdate where f_id=:3'
    using p_address,p_operator,p_row_id;
    commit;
    dbms_output.put_line('Updated address successfully.');

    --异常捕获省略
    --...
end;
/

--过程：修改客户余额
create or replace procedure modify_balance
(p_row_id in number,p_balance in number, p_operator in number)
as
begin
    --校验参数省略
    --...
    execute immediate 'update t_customer set f_balance=:1,f_modified_id=:2, f_modified_time=sysdate where f_id=:3'
    using p_balance,p_operator,p_row_id;
    commit;
    dbms_output.put_line('Updated balance successfully.');

    --异常捕获省略
    --...
end;
/

--过程：修改客户电子邮箱
create or replace procedure modify_email
(p_row_id in number,p_email in varchar2, p_operator in number)
as
begin
    --校验参数省略
    --...
    execute immediate 'update t_customer set f_email=:1,f_modified_id=:2, f_modified_time=sysdate where f_id=:3'
    using p_email,p_operator,p_row_id;
    commit;
    dbms_output.put_line('Updated email successfully.');

    --异常捕获省略
    --...
end;
/

--过程：修改客户联系人
create or replace procedure modify_linkman
(p_row_id in number,p_linkman_name in varchar2, p_operator in number)
as
begin
    --校验参数省略
    --...
    execute immediate 'update t_customer set f_linkman=:1,f_modified_id=:2, f_modified_time=sysdate where f_id=:3'
    using p_linkman_name,p_operator,p_row_id;
    commit;
    dbms_output.put_line('Updated linkman name successfully.');

    --异常捕获省略
    --...
end;
/

--过程：修改客户联系人信息
create or replace procedure modify_linkman_info
(p_row_id in number,p_linkman_name in varchar2,p_mobile in varchar2,
 p_telephone in varchar2,p_email in varchar2,p_operator in number)
as
begin
    --校验参数省略
    --...
    execute immediate 'update t_customer set f_linkman=:1, f_mobile=:2,
    f_telephone=:3, f_email=:4, f_modified_id=:5, f_modified_time=sysdate where f_id=:6'
    using p_linkman_name,p_mobile,p_telephone,p_email,p_operator,p_row_id;
    commit;
    dbms_output.put_line('Updated linkman info successfully.');

    --异常捕获省略
    --...
end;
/

--过程：修改联系手机
create or replace procedure modify_mobile
(p_row_id in number,p_mobile in varchar2,p_operator in number)
as
begin
    --校验参数省略
    --...
    execute immediate 'update t_customer set f_mobile=:1,f_modified_id=:2, f_modified_time=sysdate where f_id=:3'
    using p_mobile,p_operator,p_row_id;
    commit;
    dbms_output.put_line('Updated mobile successfully.');

    --异常捕获省略
    --...
end;
/

--过程：修改联系固话
create or replace procedure modify_telephone
(p_row_id in number,p_telephone in varchar2,p_operator in number)
as
begin
    --校验参数省略
    --...
    execute immediate 'update t_customer set f_telephone=:1,f_modified_id=:2, f_modified_time=sysdate where f_id=:3'
    using p_telephone,p_operator,p_row_id;
    commit;
    dbms_output.put_line('Updated mobile successfully.');

    --异常捕获省略
    --...
end;
/
```

### 执行

```plsql
-- 初始化
call init();

-- 更改客户信息
begin
	modify_linkman(1,'李明顺',1);
	dbms_lock.sleep(1);
	modify_mobile(1,'13771083211',2);
	dbms_lock.sleep(1);
	modify_balance(1,20020,3);
	dbms_lock.sleep(1);
	modify_address(1,'中国上海市嘉定区xxx路',4);
	dbms_lock.sleep(1);
	modify_email(1,'test1@163.com',5);
	dbms_lock.sleep(1);
	modify_telephone(1,'02183652145',6);
	dbms_lock.sleep(1);
	modify_linkman_info(1,'张雨轩','15332892301','02188881111','zhangyx@gmail.com',7);
end;
/

-- 审计
select * from T_AUDIT;

-- 回滚客户信息
-- 方法1：
update t_customer a
set(
a.f_full_name,a.f_linkman,a.f_mobile,a.f_telephone,a.f_email,a.f_address,
a.f_city,a.f_balance,a.f_partner,a.f_remark,a.f_salesman,a.f_deleted_tag,
a.f_modified_id,a.f_modified_time
)
=
(
select b.f_full_name,b.f_linkman,b.f_mobile,b.f_telephone,b.f_email,
b.f_address,b.f_city,b.f_balance,b.f_partner,b.f_remark,b.f_salesman,
b.f_deleted_tag,5,sysdate
from t_customer_history b where b.f_id=a.f_id and b.f_version=3
)
where a.f_id=1;
-- 方法2：
merge into t_customer a using t_customer_history b on (a.f_id=1 and a.f_id=b.f_id and b.f_version=3)
when matched then
update set a.f_full_name=b.f_full_name,a.f_linkman=b.f_linkman,a.f_mobile=b.f_mobile,a.f_telephone=b.f_telephone,
a.f_email=b.f_email,a.f_address=b.f_address,a.f_city=b.f_city,a.f_balance=b.f_balance,a.f_partner=b.f_partner,
a.f_remark=b.f_remark,a.f_salesman=b.f_salesman,a.f_deleted_tag=b.f_deleted_tag,a.f_modified_id=5,a.f_modified_time=sysdate;

-- 查看验证数据
select * from t_customer where f_id=1
union
select * from t_customer_history where f_id=1 and f_version=3;
```

# Lesson 3

## 创建用户并分配权限

### 创建用户

```plsql
create user permission identified by permission;
```

### 分配权限

```plsql
grant connect,resource to permission;
alter user permisson quota unlimited on users;
conn permission/permission;
```

## 创建表及其他对象

### 方案一

> 方案一在T_CUSTOMER表中存放创建人员ID，以查询该客户的直接负责人，在T_USER表中存放直属领导的ID，用于查询某领导所有下属的客户。

#### 创建表

```plsql

--方案一
--部门表
CREATE TABLE T_DEPARTMENT
(	"F_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_NAME" VARCHAR2(50 BYTE) NOT NULL ENABLE,
	"F_PARENT_ID" NUMBER(6,0),
	"F_MANAGER_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_REMARK" VARCHAR2(200 BYTE),
	 CONSTRAINT "T_DEPARTMENT_PK" PRIMARY KEY ("F_ID")
) ;

COMMENT ON COLUMN "T_DEPARTMENT"."F_ID" IS 'PK';
COMMENT ON COLUMN "T_DEPARTMENT"."F_NAME" IS '部门名称';
COMMENT ON COLUMN "T_DEPARTMENT"."F_PARENT_ID" IS '上级部门ID';
COMMENT ON COLUMN "T_DEPARTMENT"."F_MANAGER_ID" IS '部门经理';
COMMENT ON COLUMN "T_DEPARTMENT"."F_REMARK" IS '备注信息';
COMMENT ON TABLE "T_DEPARTMENT"  IS '部门表';

--用户表
CREATE TABLE "T_USER"
(	"F_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_DEPT_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_NAME" VARCHAR2(50 BYTE) NOT NULL ENABLE,
	"F_SEX" VARCHAR2(5 BYTE) DEFAULT NULL,
	"F_MOBILE" VARCHAR2(20 BYTE),
	"F_EMAIL" VARCHAR2(50 BYTE),
	"F_REMARK" VARCHAR2(200 BYTE),
	 CONSTRAINT "T_USER_PK" PRIMARY KEY ("F_ID")
);

COMMENT ON COLUMN "T_USER"."F_ID" IS 'PK';
COMMENT ON COLUMN "T_USER"."F_DEPT_ID" IS '部门ID';
COMMENT ON COLUMN "T_USER"."F_NAME" IS '用户名';
COMMENT ON COLUMN "T_USER"."F_SEX" IS '性别';
COMMENT ON COLUMN "T_USER"."F_MOBILE" IS '手机';
COMMENT ON COLUMN "T_USER"."F_EMAIL" IS '邮箱';
COMMENT ON COLUMN "T_USER"."F_REMARK" IS '备注信息';
COMMENT ON TABLE "T_USER"  IS '用户表';

--客户信息表
CREATE TABLE "T_CUSTOMER"
(	"F_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_NAME" VARCHAR2(145 BYTE) NOT NULL ENABLE,
	"F_LINKMAN" VARCHAR2(45 BYTE) NOT NULL ENABLE,
	"F_MOBILE" VARCHAR2(11 BYTE) NOT NULL ENABLE,
	"F_EMAIL" VARCHAR2(60 BYTE),
	"F_ADDRESS" VARCHAR2(200 BYTE),
	"F_REMARK" VARCHAR2(200 BYTE),
	"F_CREATED_ID" NUMBER(6,0) NOT NULL ENABLE,
	"F_CREATED_TIME" TIMESTAMP (6) NOT NULL ENABLE,
	 PRIMARY KEY ("F_ID")
);

COMMENT ON COLUMN "T_CUSTOMER"."F_ID" IS 'PK';
COMMENT ON COLUMN "T_CUSTOMER"."F_NAME" IS '客户全名';
COMMENT ON COLUMN "T_CUSTOMER"."F_LINKMAN" IS '联系人';
COMMENT ON COLUMN "T_CUSTOMER"."F_MOBILE" IS '联系手机';
COMMENT ON COLUMN "T_CUSTOMER"."F_EMAIL" IS '联系邮箱';
COMMENT ON COLUMN "T_CUSTOMER"."F_ADDRESS" IS '地址';
COMMENT ON COLUMN "T_CUSTOMER"."F_REMARK" IS '备注信息';
COMMENT ON COLUMN "T_CUSTOMER"."F_CREATED_ID" IS '创建人';
COMMENT ON COLUMN "T_CUSTOMER"."F_CREATED_TIME" IS '创建时间';
COMMENT ON TABLE "T_CUSTOMER"  IS '客户信息表';
```

#### 创建过程

##### 初始化

```plsql
--过程：数据初始化
CREATE OR REPLACE PROCEDURE INIT IS
BEGIN
    --清除数据
    EXECUTE IMMEDIATE 'TRUNCATE TABLE T_CUSTOMER';
    EXECUTE IMMEDIATE 'TRUNCATE TABLE T_USER';
    EXECUTE IMMEDIATE 'TRUNCATE TABLE T_DEPARTMENT';

    --插入部门
    INSERT INTO T_DEPARTMENT VALUES(1,'公司',NULL,1,'REMARK...');
    INSERT INTO T_DEPARTMENT VALUES(2,'行政部',1,1,'REMARK...');
    INSERT INTO T_DEPARTMENT VALUES(3,'销售部',1,2,'REMARK...');
    INSERT INTO T_DEPARTMENT VALUES(4,'电销组',3,3,'销售部电销组');
    INSERT INTO T_DEPARTMENT VALUES(5,'推销组',3,6,'销售部推销组');

    --插入用户
    INSERT INTO T_USER VALUES(1,1,'管理员','男','13771234101','USE1@SAMTECH.COM','系统管理员');
    INSERT INTO T_USER VALUES(2,3,'李明申','男','13771234102','USE2@SAMTECH.COM','销售部经理');
    INSERT INTO T_USER VALUES(3,4,'张雪', '女','13771234103','USE3@SAMTECH.COM','销售部电销组主管');
    INSERT INTO T_USER VALUES(4,4,'王刚', '男','13771234104','USE4@SAMTECH.COM','销售部电销组业务员1');
    INSERT INTO T_USER VALUES(5,4,'赵昌日','男','13771234105','USE5@SAMTECH.COM','销售部电销组业务员2');
    INSERT INTO T_USER VALUES(6,5,'孙晓华','男','13771234106','USE6@SAMTECH.COM','销售部推销组主管');
    INSERT INTO T_USER VALUES(7,5,'陈亚男','女','13771234107','USE7@SAMTECH.COM','销售部推销组业务员3');
    INSERT INTO T_USER VALUES(8,5,'刘兵超','男','13771234108','USE8@SAMTECH.COM','销售部推销组业务员4');
    INSERT INTO T_USER VALUES(9,3,'陈彬','女','13771234109','USE9@SAMTECH.COM','销售部业务员X1');
    INSERT INTO T_USER VALUES(10,3,'王军','男','13771234110','USE10@SAMTECH.COM','销售部业务员X2');

    --插入客户
    INSERT INTO T_CUSTOMER VALUES(1,'上海市永辉电子股份有限公司'     ,'张明升','15352678121','MING1@GOOGLE.COM','上海市静安区城区安泰路1108号','电销组主管创建',3,SYSDATE);
    INSERT INTO T_CUSTOMER VALUES(2,'上海博运汽车销售有限公司'      ,'朱荣荣' ,'13231289212','MING2@GOOGLE.COM','上海市徐汇区钦江路256号','电销组业务员1创建',4,SYSDATE);
    INSERT INTO T_CUSTOMER VALUES(3,'安徽广宏顶管装备制造有限公司'   ,'邱阳阳' ,'15328921231','MING3@GOOGLE.COM','安徽省广德县经济开发区东纬路5号','电销组业务员2创建',5,SYSDATE);
    INSERT INTO T_CUSTOMER VALUES(4,'上海定丰霖贸易有限公司'        ,'赵兰'  ,'15532212322','MING4@GOOGLE.COM','上海市浦东新区东延路112号408室','推销组主管创建',6,SYSDATE);
    INSERT INTO T_CUSTOMER VALUES(5,'上海东俊科技有限公司'          ,'张军'  ,'15367823660','MING5@GOOGLE.COM','上海市长宁区王安路135号','推销组业务员1创建',7,SYSDATE);
    INSERT INTO T_CUSTOMER VALUES(6,'中科创客（深圳）智能工业设备公司','李明'  ,'17723180234','MING6@GOOGLE.COM','深圳市龙岗区富民工业园致康路301号','推销组业务员2创建',8,SYSDATE);
    INSERT INTO T_CUSTOMER VALUES(7,'南宁云讯科技有限公司'          ,'王永成','13568932166','MING7@GOOGLE.COM','广东省深圳市福田区长川路102号','销售部业务员X1创建',9,SYSDATE);
    INSERT INTO T_CUSTOMER VALUES(8,'沈阳优速家政服务有限公司'       ,'李东升','13392312343','MING8@GOOGLE.COM','辽宁省沈阳市铁西区北二路青年易居东门32号','销售部业务员X2创建',10,SYSDATE);
    COMMIT;
END;
/
```

#### 执行

##### 初始化

```
set serveroutput on;
exec init;
```

##### 查询自己的客户

```plsql
SELECT * FROM t_customer A WHERE A.f_created_id=&id;
```

> `&id`是所查询人员的ID

##### 查询某领导下属人员的所有客户

```plsql
select * from t_user a  where exists(
SELECT 1 FROM t_department b
WHERE a.f_dept_id=b.f_id and b.f_manager_id=&id
CONNECT BY b.F_PARENT_ID = PRIOR b.F_ID
start with b.F_ID = (select c.f_dept_id from t_user c where c.f_id=&id));
```

> `&id`是该领导的ID

#### 当部门结构或员工归属调整时，权限编码如何处理？

对于方案一，只需要更改员工直属领导ID即可

### 方案二

> 方案二取消在T_USER中添加直属领导ID，改为在员工、部门、客户表中添加权限码，查看时直接搜索对应权限码即可

#### 创建表

```plsql
--方案二
--部门表
CREATE TABLE T_DEPARTMENT_2
(  "F_ID" NUMBER(6,0) NOT NULL ENABLE,
 "F_NAME" VARCHAR2(50 BYTE) NOT NULL ENABLE,
    "F_CODE" VARCHAR2(50 BYTE) NOT NULL ENABLE,
 "F_PARENT_ID" NUMBER(6,0),
 "F_MANAGER_ID" NUMBER(6,0) NOT NULL ENABLE,
 "F_REMARK" VARCHAR2(200 BYTE),
  CONSTRAINT "T_DEPARTMENT_PK2" PRIMARY KEY ("F_ID")
) ;

COMMENT ON COLUMN "T_DEPARTMENT_2"."F_ID" IS 'PK';
COMMENT ON COLUMN "T_DEPARTMENT_2"."F_NAME" IS '部门名称';
COMMENT ON COLUMN "T_DEPARTMENT_2"."F_CODE" IS '部门编码';
COMMENT ON COLUMN "T_DEPARTMENT_2"."F_PARENT_ID" IS '上级部门ID';
COMMENT ON COLUMN "T_DEPARTMENT_2"."F_MANAGER_ID" IS '部门经理';
COMMENT ON COLUMN "T_DEPARTMENT_2"."F_REMARK" IS '备注信息';
COMMENT ON TABLE "T_DEPARTMENT_2"  IS '部门表2';

--用户表
CREATE TABLE "T_USER_2"
(  "F_ID" NUMBER(6,0) NOT NULL ENABLE,
 "F_DEPT_ID" NUMBER(6,0) NOT NULL ENABLE,
 "F_NAME" VARCHAR2(50 BYTE) NOT NULL ENABLE,
    "F_CODE" VARCHAR2(50 BYTE) NOT NULL ENABLE,
 "F_SEX" VARCHAR2(5 BYTE) DEFAULT NULL,
 "F_MOBILE" VARCHAR2(20 BYTE),
 "F_EMAIL" VARCHAR2(50 BYTE),
 "F_REMARK" VARCHAR2(200 BYTE),
  CONSTRAINT "T_USER_PK2" PRIMARY KEY ("F_ID")
);

COMMENT ON COLUMN "T_USER_2"."F_ID" IS 'PK';
COMMENT ON COLUMN "T_USER_2"."F_DEPT_ID" IS '部门ID';
COMMENT ON COLUMN "T_USER_2"."F_NAME" IS '用户名';
COMMENT ON COLUMN "T_USER_2"."F_CODE" IS '用户编码';
COMMENT ON COLUMN "T_USER_2"."F_SEX" IS '性别';
COMMENT ON COLUMN "T_USER_2"."F_MOBILE" IS '手机';
COMMENT ON COLUMN "T_USER_2"."F_EMAIL" IS '邮箱';
COMMENT ON COLUMN "T_USER_2"."F_REMARK" IS '备注信息';
COMMENT ON TABLE "T_USER_2"  IS '用户表2';

--客户信息表
CREATE TABLE "T_CUSTOMER_2"
(  "F_ID" NUMBER(6,0) NOT NULL ENABLE,
 "F_NAME" VARCHAR2(145 BYTE) NOT NULL ENABLE,
 "F_LINKMAN" VARCHAR2(45 BYTE) NOT NULL ENABLE,
 "F_MOBILE" VARCHAR2(11 BYTE) NOT NULL ENABLE,
 "F_EMAIL" VARCHAR2(60 BYTE),
 "F_ADDRESS" VARCHAR2(200 BYTE),
 "F_REMARK" VARCHAR2(200 BYTE),
    "F_ACCESS_CODE" VARCHAR2(50 BYTE) NOT NULL ENABLE,
 "F_CREATED_ID" NUMBER(6,0) NOT NULL ENABLE,
 "F_CREATED_TIME" TIMESTAMP (6) NOT NULL ENABLE,
  PRIMARY KEY ("F_ID")
);

COMMENT ON COLUMN "T_CUSTOMER_2"."F_ID" IS 'PK';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_NAME" IS '客户全名';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_LINKMAN" IS '联系人';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_MOBILE" IS '联系手机';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_EMAIL" IS '联系邮箱';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_ADDRESS" IS '地址';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_REMARK" IS '备注信息';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_ACCESS_CODE" IS '权限编码';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_CREATED_ID" IS '创建人';
COMMENT ON COLUMN "T_CUSTOMER_2"."F_CREATED_TIME" IS '创建时间';
COMMENT ON TABLE "T_CUSTOMER_2"  IS '客户信息表2';

-- 创建人员更改历史表
CREATE TABLE T_USER_HISTORY(
  ID NUMBER(6,0) PRIMARY KEY ,
  F_ID NUMBER(6,0) ,
  O_DEP_ID NUMBER(6,0) ,
  O_ACCESS_CODE VARCHAR2(50 BYTE) ,
  N_DEP_ID  NUMBER(6,0),
  N_ACCESS_CODE VARCHAR2(50 BYTE),
  TIME DATE
);

COMMENT ON COLUMN T_USER_HISTORY.ID IS 'PK';
COMMENT ON COLUMN T_USER_HISTORY.F_ID IS '修改的人员ID';
COMMENT ON COLUMN T_USER_HISTORY.O_DEP_ID IS '旧的部门';
COMMENT ON COLUMN T_USER_HISTORY.O_ACCESS_CODE IS '旧的权限';
COMMENT ON COLUMN T_USER_HISTORY.N_DEP_ID IS '新的部门';
COMMENT ON COLUMN T_USER_HISTORY.N_ACCESS_CODE IS '新的权限';
COMMENT ON TABLE T_USER_HISTORY IS '用户权限更改历史';
```

#### 创建过程

##### 初始化

```plsql
--过程：数据初始化
CREATE OR REPLACE PROCEDURE INIT2 IS
BEGIN
    --清除数据
    EXECUTE IMMEDIATE 'TRUNCATE TABLE T_CUSTOMER_2';
    EXECUTE IMMEDIATE 'TRUNCATE TABLE T_USER_2';
    EXECUTE IMMEDIATE 'TRUNCATE TABLE T_DEPARTMENT_2';
    EXECUTE IMMEDIATE 'TRUNCATE TABLE T_USER_HISTORY';

    --插入部门
    INSERT INTO T_DEPARTMENT_2 VALUES(1,'公司','1',NULL,1,'REMARK...');
    INSERT INTO T_DEPARTMENT_2 VALUES(2,'行政部','101',1,1,'REMARK...');
    INSERT INTO T_DEPARTMENT_2 VALUES(3,'销售部','102',1,2,'REMARK...');
    INSERT INTO T_DEPARTMENT_2 VALUES(4,'电销组','10201',3,3,'销售部电销组');
    INSERT INTO T_DEPARTMENT_2 VALUES(5,'推销组','10202',3,6,'销售部推销组');

    --插入用户
    INSERT INTO T_USER_2 VALUES(1,1,'管理员','1','男','13771234101','USE1@SAMTECH.COM','系统管理员');
    INSERT INTO T_USER_2 VALUES(2,3,'李明申','102','男','13771234102','USE2@SAMTECH.COM','销售部经理');
    INSERT INTO T_USER_2 VALUES(3,4,'张雪',  '10201', '女','13771234103','USE3@SAMTECH.COM','销售部电销组主管');
    INSERT INTO T_USER_2 VALUES(4,4,'王刚',  '1020101', '男','13771234104','USE4@SAMTECH.COM','销售部电销组业务员1');
    INSERT INTO T_USER_2 VALUES(5,4,'赵昌日','1020102','男','13771234105','USE5@SAMTECH.COM','销售部电销组业务员2');
    INSERT INTO T_USER_2 VALUES(6,5,'孙晓华','10202','男','13771234106','USE6@SAMTECH.COM','销售部推销组主管');
    INSERT INTO T_USER_2 VALUES(7,5,'陈亚男','1020201','女','13771234107','USE7@SAMTECH.COM','销售部推销组业务员3');
    INSERT INTO T_USER_2 VALUES(8,5,'刘兵超','1020202','男','13771234108','USE8@SAMTECH.COM','销售部推销组业务员4');
    INSERT INTO T_USER_2 VALUES(9,3,'陈彬',  '10203','女','13771234109','USE9@SAMTECH.COM','销售部业务员X1');
    INSERT INTO T_USER_2 VALUES(10,3,'王军', '10204','男','13771234110','USE10@SAMTECH.COM','销售部业务员X2');

    --插入客户
    INSERT INTO T_CUSTOMER_2 VALUES(1,'上海市永辉电子股份有限公司'     ,'张明升','15352678121','MING1@GOOGLE.COM','上海市静安区城区安泰路1108号','电销组主管创建','10201',3,SYSDATE);
    INSERT INTO T_CUSTOMER_2 VALUES(2,'上海博运汽车销售有限公司'      ,'朱荣荣' ,'13231289212','MING2@GOOGLE.COM','上海市徐汇区钦江路256号','电销组业务员1创建','1020101',4,SYSDATE);
    INSERT INTO T_CUSTOMER_2 VALUES(3,'安徽广宏顶管装备制造有限公司'   ,'邱阳阳' ,'15328921231','MING3@GOOGLE.COM','安徽省广德县经济开发区东纬路5号','电销组业务员2创建','1020102',5,SYSDATE);
    INSERT INTO T_CUSTOMER_2 VALUES(4,'上海定丰霖贸易有限公司'        ,'赵兰'  ,'15532212322','MING4@GOOGLE.COM','上海市浦东新区东延路112号408室','推销组主管创建','10202',6,SYSDATE);
    INSERT INTO T_CUSTOMER_2 VALUES(5,'上海东俊科技有限公司'          ,'张军'  ,'15367823660','MING5@GOOGLE.COM','上海市长宁区王安路135号','推销组业务员1创建','1020201',7,SYSDATE);
    INSERT INTO T_CUSTOMER_2 VALUES(6,'中科创客（深圳）智能工业设备公司','李明'  ,'17723180234','MING6@GOOGLE.COM','深圳市龙岗区富民工业园致康路301号','推销组业务员2创建','1020202',8,SYSDATE);
    INSERT INTO T_CUSTOMER_2 VALUES(7,'南宁云讯科技有限公司'          ,'王永成','13568932166','MING7@GOOGLE.COM','广东省深圳市福田区长川路102号','销售部业务员X1创建','10203',9,SYSDATE);
    INSERT INTO T_CUSTOMER_2 VALUES(8,'沈阳优速家政服务有限公司'       ,'李东升','13392312343','MING8@GOOGLE.COM','辽宁省沈阳市铁西区北二路青年易居东门32号','销售部业务员X2创建','10204',10,SYSDATE);
    COMMIT;
END;
/
```

##### 将某员工调换到某部门

![image-20181126025537963](/images/image-20181126025537963.png)

```plsql
-- 更改用户到特定部门
CREATE OR REPLACE PROCEDURE CHANGE_TO_DEPARTMENT(C_F_ID IN T_USER_2.F_ID%TYPE, N_DEP_ID IN T_DEPARTMENT_2.F_ID%TYPE) IS
  -- 旧部门
  O_DEP_ID T_DEPARTMENT_2.F_ID%TYPE;
  -- 旧权限
  O_ACCESS_CODE T_USER_2.F_CODE%TYPE;
  -- 部门权限前缀
  DEP_ACCESS_CODE_PREFIX T_DEPARTMENT_2.F_CODE%TYPE;
  -- 部门当前人数
  DEP_USER_COUNT T_USER_2.F_CODE%TYPE;
  -- 本部门下的部门数
  DEP_DEP_COUNT T_DEPARTMENT_2.F_CODE%TYPE;
  -- 新权限
  N_ACCESS_CODE T_USER_2.F_CODE%TYPE;
  -- 更新该员工权限
  C_UPDATE_USER VARCHAR2(100) := 'UPDATE T_USER_2 SET F_CODE = :1, F_DEPT_ID = :2 WHERE F_ID = :3';
  -- 更新所有该员工的客户的ACCESS权限
  C_UPDATE_CUSTOMER VARCHAR2(100) := 'UPDATE T_CUSTOMER_2 SET F_ACCESS_CODE = :1 WHERE F_CREATED_ID = :2';
  -- 插入一条修改记录
  C_INSERT_HISTORY VARCHAR2(100) := 'INSERT INTO T_USER_HISTORY VALUES (:1, :2, :3, :4, :5, :6, :7)';
  BEGIN
    -- 旧部门
    SELECT F_DEPT_ID INTO O_DEP_ID FROM T_USER_2 WHERE T_USER_2.F_ID=C_F_ID;
    -- 旧权限
    SELECT F_CODE INTO O_ACCESS_CODE FROM T_USER_2 WHERE T_USER_2.F_ID=C_F_ID;
    -- 新部门权限作为前缀
    SELECT F_CODE INTO DEP_ACCESS_CODE_PREFIX FROM T_DEPARTMENT_2 WHERE T_DEPARTMENT_2.F_ID = N_DEP_ID;
    -- 计算该部门人员数量
    SELECT MAX(T_USER_2.F_CODE) INTO DEP_USER_COUNT FROM T_USER_2 WHERE T_USER_2.F_DEPT_ID = N_DEP_ID;
    -- 计算子部门数量
    SELECT MAX(T_DEPARTMENT_2.F_CODE) INTO DEP_DEP_COUNT FROM T_DEPARTMENT_2 WHERE SUBSTR(T_DEPARTMENT_2.F_CODE, 0, LENGTH(DEP_ACCESS_CODE_PREFIX))=DEP_ACCESS_CODE_PREFIX AND LENGTH(T_DEPARTMENT_2.F_CODE)=LENGTH(DEP_ACCESS_CODE_PREFIX)+2;
    -- 若新部门与旧部门相同，无需更改
    IF N_DEP_ID=O_DEP_ID THEN
      RETURN;
    END IF;
    -- 新权限CODE
    IF DEP_DEP_COUNT > DEP_USER_COUNT THEN
      N_ACCESS_CODE := TO_CHAR(TO_NUMBER(DEP_DEP_COUNT) + 1);
    ELSE
      N_ACCESS_CODE := TO_CHAR(TO_NUMBER(DEP_USER_COUNT) + 1);
    end if;
    -- 输出相关变量
    dbms_output.put_line('DEP_USER_COUNT : ' || DEP_USER_COUNT);
    dbms_output.put_line('DEP_DEP_COUNT : ' || DEP_DEP_COUNT);
    -- 输出相关变量
    dbms_output.put_line('DEP_ACCESS_CODE_PREFIX : ' || DEP_ACCESS_CODE_PREFIX);
    dbms_output.put_line('C_F_ID : ' || C_F_ID);
    dbms_output.put_line('O_ACCESS_CODE : ' || O_ACCESS_CODE);
    dbms_output.put_line('N_DEP_ID : ' || N_DEP_ID);
    dbms_output.put_line('N_ACCESS_CODE : ' || N_ACCESS_CODE);
    -- 更新该员工权限
    EXECUTE IMMEDIATE C_UPDATE_USER USING N_ACCESS_CODE, N_DEP_ID, C_F_ID;
    -- 更新所有该员工的客户的ACCESS权限
    EXECUTE IMMEDIATE C_UPDATE_CUSTOMER USING N_ACCESS_CODE, C_F_ID;
    -- 插入一条修改记录
    EXECUTE IMMEDIATE C_INSERT_HISTORY USING USER_HISTORY.NEXTVAL, C_F_ID, O_DEP_ID, O_ACCESS_CODE, N_DEP_ID, N_ACCESS_CODE, SYSDATE;
    -- 提交
    COMMIT;
  END;
  /
```

#### 创建序列

```plsql
-- 员工部门历史记录主码序列
CREATE SEQUENCE USER_HISTORY INCREMENT BY 1 START WITH 1;
```

#### 执行

```plsql
select * from t_customer_2 where f_access_code like 'xxx%';
```

> `xxx%`指匹配所有以`xxx`开头的权限码

#### 当部门结构或员工归属调整时，权限编码如何处理？

方案二中，调用新增的过程`CHANGE_TO_DEPARTMENT`即可级联更改权限码。
