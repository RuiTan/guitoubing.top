---
title: C# - ADO.NET配置说明
date: 2019-04-18 11:04:15
tags:
    - .NET
    - ADO.NET
categories:
    - C#
---

## 概述

`ADO.NET`提供对数据库如`MySQL`和`XML`这样的数据源以及通过`OLE DB`和`ODBC`公开的数据源的一致访问。共享数据的使用方应用程序可以使用`ADO.NET`连接到这些数据源，并可以检索、处理和更新其中包含的数据。

<!-- more -->

## 安装MySQL数据库

### 创建数据库

```shell
PS A:\> cd .\MySQL\mysql-5.7.25-winx64\bin\

# 安装MySQL服务
PS A:\MySQL\mysql-5.7.25-winx64\bin> .\mysqld install

# 初始化MySQL数据库，创建无root密码的root用户
PS A:\MySQL\mysql-5.7.25-winx64\bin> mysqld --initialize-insecure

# 启动MySQL服务
PS A:\MySQL\mysql-5.7.25-winx64\bin> net start mysql
MySQL 服务正在启动 .
MySQL 服务已经启动成功。

# 创建用户，设置密码并赋予权限
PS A:\MySQL\mysql-5.7.25-winx64\bin> mysql -uroot
Welcome to the MySQL monitor.  Commands end with ; or \g.
Your MySQL connection id is 2
Server version: 5.7.25 MySQL Community Server (GPL)

Copyright (c) 2000, 2019, Oracle and/or its affiliates. All rights reserved.

Oracle is a registered trademark of Oracle Corporation and/or its
affiliates. Other names may be trademarks of their respective
owners.

Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.
# 创建用户并设置密码
mysql> create user 'tanrui'@'localhost' identified by 'tanrui';
Query OK, 0 rows affected (0.00 sec)
# 赋予权限
mysql> grant all privileges on *.* to 'tanrui'@'localhost';
Query OK, 0 rows affected (0.00 sec)

mysql> exit
Bye
# 以新用户登录数据库
PS A:\MySQL\mysql-5.7.25-winx64\bin> mysql -utanrui -ptanrui
mysql: [Warning] Using a password on the command line interface can be insecure.
Welcome to the MySQL monitor.  Commands end with ; or \g.
Your MySQL connection id is 7
Server version: 5.7.25 MySQL Community Server (GPL)

Copyright (c) 2000, 2019, Oracle and/or its affiliates. All rights reserved.

Oracle is a registered trademark of Oracle Corporation and/or its
affiliates. Other names may be trademarks of their respective
owners.

Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.
# 创建数据库
mysql> create database Assignment4;
Query OK, 1 row affected (0.00 sec)

mysql> show databases;
+--------------------+
| Database           |
+--------------------+
| information_schema |
| assignment4        |
| mysql              |
| performance_schema |
| sys                |
+--------------------+
5 rows in set (0.00 sec)
```

### 使用DataGrip连接数据库

设置连接参数如下，点击`OK`完成连接：

![1555227842703](/images/1555227842703.png)

右键`assignment4`数据库点击`New`->`Table`创建`Student`和`Score`新表：

![1555227964411](/images/1555227964411.png)

![1555228253075](/images/1555228253075.png)

![1555228331186](/images/1555228331186.png)

## Visual Studio Nuget包管理

本次实验需要用到`MySQL`一些操作，因此需要在`Visual Studio`中添加相应的包，如下：

![image-20190414160931753](/images/image-20190414160931753.png)

打开`Nuget包管理器`，下载`Mysql.Data`和`System.Data.DataSetExtensions`两个包，前者提供`MySQL`连接驱动，后者主要是提供`C#`中对于数据库表操作的一些语法支持。

![image-20190414161020200](/images/image-20190414161020200.png)

## ADONET基础编程

### 一个小示例

```c#
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data;
using MySql.Data.MySqlClient;

namespace ADONET
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Hello World!");
            //ADO.NET 几个关键对象
            //(1) Connection对象 用于连接数据库
            //(2) Command对象 对数据源执行命令
            //(3) DataReader对象 用户从数据源读取数据
            //(4) DataAdapter对象 从数据源读取数据并且填充数据集对象 
            //(5) DataSet对象 相当于内存数据库
            //(6) DataTable对象 相当于内存数据库中的表格
            string connString = "server = 127.0.0.1; user id = tanrui; password = tanrui; persistsecurityinfo = True; database = assignment4";
            MySqlConnection conn = new MySqlConnection(connString);
            conn.Open();

            MySqlCommand cmd = new MySqlCommand();
            cmd.Connection = conn;
            //ExecuteNonQuery的简单使用
                        cmd.CommandText = "insert into Student values (1,'zhangsan1',20)";
                        cmd.ExecuteNonQuery();
                        cmd.CommandText = "insert into Student values (2,'zhangsan2',19)";
                        cmd.ExecuteNonQuery();
                        cmd.CommandText = "insert into Student values (3,'zhangsan3',22)";
                        cmd.ExecuteNonQuery();
                        cmd.CommandText = "insert into Score values (1,80,85)";
                        cmd.ExecuteNonQuery();
                        cmd.CommandText = "insert into Score values (2,90,95)";
                        cmd.ExecuteNonQuery();
                        cmd.CommandText = "insert into Score values (3,86,75)";
                        cmd.ExecuteNonQuery();
                       
            //参数化查询的使用，是目前唯一可以预防SQL Injection的方法
            cmd.CommandText = @"INSERT INTO Student (Id,Name,Age) VALUES (@Id,@Name,@Age)";
                        cmd.Parameters.Add(new MySqlParameter("@Id", 100));
                        cmd.Parameters.Add(new MySqlParameter("@Name",
            "newstudent"));
                        cmd.Parameters.Add(new MySqlParameter("@Age", 18));
                        cmd.ExecuteNonQuery();
            //ExecuteReader的简单使用
            cmd.CommandText = "select Count(*) from Student";
            object nRows = cmd.ExecuteScalar();
            Console.WriteLine("nRows: {0}", nRows);

cmd.CommandText = "select * from Student";
            MySqlDataReader dr = cmd.ExecuteReader();
            while(dr.Read())
            {
                 Console.WriteLine("One Row: {0},{1},{2}",
dr.GetValue(0), dr["Name"], dr.GetValue( dr.GetOrdinal("Age") ));
            }
            dr.Close();
            //使用MySqlDataAdapter填充DataSet
            DataSet ds = new DataSet();
            MySqlDataAdapter adapter = new MySqlDataAdapter(cmd);
            cmd.CommandText = "select * from Student";
            adapter.Fill(ds, "Student");
            cmd.CommandText = "select * from Score";
            adapter.Fill(ds, "Score");
            DataTable Student = ds.Tables["Student"];
            DataTable Score = ds.Tables["Score"];
            //LINQ与DataTable的结合
            var qry = from s in Student.AsEnumerable()
                      join c in Score.AsEnumerable() on s.Field<int>("Id") equals c.Field<int>("StudentId")
                      where c.Field<int>("English") > 80
                      select new
                      {
                          Name = s.Field<string>("Name"),
                          English = c.Field<int>("English"),
                          Maths = c.Field<int>("Maths")
                      };
            foreach (var item in qry)
            {
                Console.WriteLine(item);
            }
            //可以使用using模式进行资源的释放
            conn.Close();
        }
    }
}
/* Output:
Hello World!
nRows: 4
One Row: 1,zhangsan1,20
One Row: 2,zhangsan2,19
One Row: 3,zhangsan3,22
One Row: 100,newstudent,18
{ Name = zhangsan2, English = 90, Maths = 95 }
{ Name = zhangsan3, English = 86, Maths = 75 }
*/~
```

上述例子将ADONET中几个主要对象综合到了一起，下面逐一介绍各个对象的功能及用法。

### Connection

`Connection对象`用于和数据库交互，若要操作数据库必须与其连接。创建连接时需要指定`数据库服务器`、`数据库名`、`用户名`、`密码`以及`其他所需参数`。例如本例中的`connString`：

```c#
string connString = "server = 127.0.0.1; user id = tanrui; password = tanrui; persistsecurityinfo = 	True; database = assignment4";
```

每一种数据源都有特定的`Connection类`，例如本例中的`MySqlConnection`：

```c#
MySqlConnection conn = new MySqlConnection(connString);
```

与`Java`不同，`C#`中创建的`Connection对象`是使用正常的构造函数创建一个连接，而`Java`中使用的是**单例模式**创建数据库连接。

### Command

`Command对象`针对`Connection对象`指定的数据源执行SQL语句和存储过程及函数。这个对象是架构在`Connection对象`上的，也就是说`Command对象`通过`Connection对象`操作数据源。

同样，对于每一种数据源都有特定的`Command类`，例如本例中：

```c#
MySqlCommand cmd = new MySqlCommand();
cmd.Connection = conn;
```

在创建了`MySqlCommand对象`后还需为其指定对应的`Connection对象`。

#### 增删改操作

`ExecuteNonQuery()`方法主要用于`Command对象`的`增删改`操作。

`Command对象`的基本使用方法如例中所示：

```c#
cmd.CommandText = "insert into Student values (1,'zhangsan1',20)";
cmd.ExecuteNonQuery();
```

先指定其`CommandText`即SQL语句，而后调用`ExecuteNonQuery()`方法完成query。

上述SQL操作方法很容易被非法分子通过字符串拼接等方式进行`SQL注入攻击`，我们可以使用`Parameters`修饰SQL语句，以防止`SQL注入攻击`：

```c#
cmd.CommandText = @"INSERT INTO Student (Id,Name,Age) VALUES (@Id,@Name,@Age)";
cmd.Parameters.Add(new MySqlParameter("@Id", 100));
cmd.Parameters.Add(new MySqlParameter("@Name", "newstudent"));
cmd.Parameters.Add(new MySqlParameter("@Age", 18));
cmd.ExecuteNonQuery();
```

#### 查询操作

查询由于返回的结果的复杂性，一般有`ExecuteScalar()`和`ExecuteReader()`两种方法。

`ExecuteScalar()`主要用于**SQL查询语句只返回一个数据**，例如`Count操作`、`Max操作`等的返回值：

```c#
cmd.CommandText = "select Count(*) from Student";
object nRows = cmd.ExecuteScalar();
Console.WriteLine("nRows: {0}", nRows);
```

`ExecuteReader()`返回一个`DataReader对象`，即结果集，下面会重点讨论该对象。

### DataReader

`DataReader对象`是对数据源的查询结果的**基于流的、仅向前的只读检索**，它不会更新数据。

它是基于`Command对象`的，意即只能通过调用`特定Command对象`的`ExecuteReader()`方法以获取`DataReader对象`：

```c#
MySqlDataReader dr = cmd.ExecuteReader();
while(dr.Read())
{
         Console.WriteLine("One Row: {0},{1},{2}",
         										dr.GetValue(0),
         										dr["Name"],
         										dr.GetValue(dr.GetOrdinal("Age") 
         									));
}
dr.Close();
```

同样，特定的数据源也是有特定类型的`DataReader类`，其都是通过继承`DbDataReader类`实现的。

由于`Read()`方法返回是否有下一行的布尔值，因此很适合使用`while循环`遍历结果。

从代码中易知，`DataReader对象`的`Read()`方法是通过循环从数据源中每次获取一行数据，类似于plsql中的游标，他除了读取效率很高之外，牺牲了很多其他特性，例如对结果的排序、更改等。

### DataSet和DataAdapter

`DataSet类`包含数据的数据表集合。它用于在**不与数据源交互**的情况下获取数据，这就是为什么它也被称为`断开数据访问方法`。这是一个**内存数据存储**，可以**同时容纳多个表**。可以使用`DataRelation`对象来关联这些表。 `DataSet`也可以用来读写`XML文档`中的数据。

`DataAdapter`是`ADO.NET`数据提供程序的一部分。`DataAdapter`提供**数据集和数据源之间的通信**。我们可以将`DataAdapter`与`DataSet`对象结合使用。注意`DataAdapter类`也是**各个数据源有各自的实现方法**。

`DataAdapter`通过映射`Fill()`方法提供此组合，该方法更改`DataSet`中的数据以匹配数据源中的数据。也就是说，这两个对象组合起来以实现`数据访问`和`数据操作`功能：

```c#
DataSet ds = new DataSet();
SqlDataAdapter adapter = new SqlDataAdapter(cmd); cmd.CommandText = "select * from Student"; adapter.Fill(ds, "Student");
cmd.CommandText = "select * from Score"; adapter.Fill(ds, "Score");
DataTable Student = ds.Tables["Student"]; DataTable Score = ds.Tables["Score"];
```

通过代码，我们可以看到`DataSet`是**独立于数据源的数据集**，即**对于任何数据源，都提供一致的关系编程模型**。

相比于`DataReader`，`DataSet`**一次性将所有数据放入内存中**，同时还提供了很多额外的数据集操作方法，因此**速度很快且很方便**，但是**对内存资源会有很大的消耗**。

### DataTable

`DataTable`类将关系数据表示为表格形式。`ADO.NET`提供了一个`DataTable`类来独立创建和使用数据表。一般与`DataSet`一起使用。 

在创建`DataTable`之前，**必须包含`System.Data`名称空间**。同时由于本例中使用了`AsEnumerable()`方法，因此还**必须使用`Nuget`添加`System.Data.DataSetExtensions`包**，**否则会报错**。

我们可以使用LINQ语法对DataTable进行查询，实现代码如下：

```c#
var qry = from s in Student.AsEnumerable()
					join c in Score.AsEnumerable() on s.Field<int>("Id") equals c.Field<int>("StudentId")
					where c.Field<int>("English") > 80
          select new
          {
                Name = s.Field<string>("Name"),
                English = c.Field<int>("English"),
                Maths = c.Field<int>("Maths")
          };
foreach (var item in qry)
{
		Console.WriteLine(item);
}
/* Output:
{ Name = zhangsan2, English = 90, Maths = 95 }
{ Name = zhangsan3, English = 86, Maths = 75 }
**//~
```
