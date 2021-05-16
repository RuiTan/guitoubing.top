---
title: C# - .NETFramework及EntityFramework
date: 2019-04-18 11:07:39
tags:
    - .NET Framework
    - EntityFramework
categories:
    - C#
---

## 概述

`EntityFramework`是一种对象关系映射器`ORM`，它使`.NET`开发人员能够使用`.NET`对象处理数据库。它消除了开发人员通常需要编写的大多数数据访问代码的需要。

<!-- more -->

## 环境配置

本次实验我使用最新的`Microsoft Visual Studio Community 2019 `版本进行实验，环境配置过程中也是历经坎坷，在此记录如下。

### VS2019连接MySQL

使用VS2019连接MySQL时需要两个必备的程序：

- [mysql-for-visualstudio-1.2.8.msi](https://cdn.mysql.com//Downloads/MySQL-for-VisualStudio/mysql-for-visualstudio-1.2.8.msi)：`MySQL for Visual Studio`是`MySQL`提供给`Microsoft Visual Studio`的驱动，用来实现对MySQL对象和数据的访问。

- [mysql-connector-net-6.10.8.msi](https://cdn.mysql.com//Downloads/Connector-Net/mysql-connector-net-6.10.8.msi)：`MySQL Connector / NET`能够开发需要与`MySQL`进行安全，高性能数据连接的.NET应用程序。它实现了所需的`ADO.NET`接口，并集成到`ADO.NET`感知工具中。

下载并安装完成之后，启动`VS2019`，选择`工具`->`连接到数据库`：

![1555337368613](/images/1555337368613.png)

选择`MySQL Database`数据源：

![1555337410546](/images/1555337410546.png)

填入我在`ADO.NET实验`中创建的数据库参数如下：

![1555337487973](/images/1555337487973.png)

至此，关于`VS2019`连接`MySQL`的配置已经完成。

### 关于创建的项目类型

在`.NET`家族中有`.NET Core`和`.NET Framework`两大平台，他们的区别如下：

**.NET Framework**：支持`Windows`和`Web`应用程序。今天，您可以使用`Windows`窗体，`WPF`和`UWP`在`.NET Framework`中构建`Windows`应用程序。`ASP.NET MVC`用于在`.NET Framework`中构建`Web应用程序`。

**.NET Core**：是一种新的开源和跨平台框架，用于为包括`Windows`，`Mac`和`Linux`在内的所有操作系统构建应用程序。`.NET Core`仅支持`UWP`和`ASP.NET Core`。`UWP`用于构建`Windows 10`目标`Windows`和移动应用程序。`ASP.NET Core`用于构建基于浏览器的`Web应用程序`。 

而在`VS2019`中也有两大类项目类型：

- .NET Core应用

  ![1555335982957](/images/1555335982957.png)

- .NET Framework应用

  ![1555336042119](/images/1555336042119.png)

而我们本次实验一定要选择`.NET Framework`应用，否则无法通过`EntityFramework`创建实体数据模型。

### 创建项目

这里我选择的是`控制台应用(.NET Framework)`：

![1555336181441](/images/1555336181441.png)

注意最后一项`框架`最好选择`4.6.0`以上的框架，这里我选择最新版本`4.7.2`。

### NuGet添加程序包

![1555336839432](/images/1555336839432.png)

使用NuGet添加程序包时最要紧的就是程序包的版本问题。

**注意！！！：**在前面安装MySQL Connector / NET时，选择的版本是6.10.8，因此我们这里的MySql.Data和MySql.Data.Entity两个程序包的版本也需要一致为`v6.10.8`，否则会出现各式各样的错误，我遇到的错误有以下两个：

1. ![1555337786084](/images/1555337786084.png)

2. 在1问题解决后，点击下一步会闪退。

至此环境配置已完成，下面针对EntityFramework三种模型来说明。

## DB First（来自数据库的EF设计器）

右键项目解决方案，选择`添加`->`新建项`：

![1555338066598](/images/1555338066598.png)

选择`ADO.NET实体数据模型`，单击`下一步`：

![1555338108853](/images/1555338108853.png)

选择`来自数据库的EF设计器`，单击`下一步`：

![1555338141910](/images/1555338141910.png)

点击`是，在连接字符串中包含敏感数据`，单击`下一步`：

![1555338199372](/images/1555338199372.png)

在`表`选项前打勾，单击完成：

![1555338281068](/images/1555338281068.png)

完成后会发现，项目解决方案中出现了`Model1.edmx`文件，里面有两种主要的类`DBContext`和`DBSet`：

![1555338670472](/images/1555338670472.png)

### DbSet

`DbSet`对应于数据库中的表，意即每个实体表对应一个`DbSet`实体类，相当于数据的集合，可通过此类间接对数据库表进行`ACID操作`再通过`DbContext`关联到数据库。具体操作再介绍完`DbContext`后一齐举例。

### DbContext

 `DbContext`类是实体框架的重要组成部分。它是您的域或实体类与数据库之间的桥梁。

![1555338761379](/images/1555338761379.png)

`DbContext`是负责与数据交互作为对象的主要类。`DbContext`负责以下活动：

1. `EntitySet`： `DbContext`包含映射到数据库表的所有实体的实体集（`DbSet <TEntity>`）。
2. 查询（Querying）： `DbContext`将`LINQ-to-Entities`查询转换为`SQL查询`并将其发送到数据库。
3. 更改跟踪（Change Tracking）：跟踪实体在从数据库查询后发生的更改。
4. 持久数据（Persisting Data）：它还根据实体的状态对数据库执行插入，更新和删除操作。
5. 缓存（Caching）： DbContext默认进行一级缓存。它存储在上下文类生命周期中已经被检索的实体。
6. 管理关系（Manage Relationship）： `DbContext`还使用`DB-First`或`Model-First`方法使用`CSDL`，`MSL`和`SSDL`或者使用`Code-First`方法使用流利的`API`来管理关系。
7. 对象实现（Object Materialization）： `DbContext`将原始表数据转换为实体对象。

下面是使用`DBContext`的示例：

```c#
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EF
{
    class Program
    {
        static void Main(string[] args)
        {
            assignment4Entities entities = new assignment4Entities();

            // 增
            student student = new student
            {
                Id = 4,
                Name = "Tanrui",
                Age = 23
            };
            entities.student.Add(student);
            student = new student
            {
                Id = 5,
                Name = "TanRui",
                Age = 23
            };
            entities.student.Add(student);
            entities.SaveChanges();

            // 删
            foreach (var stu in entities.student.Where(stu => stu.Name == "Tanrui1"))
            {
                entities.student.Remove(stu);
            }
            entities.SaveChanges();

            // 改
            foreach ( var stu in entities.student.Where(stu => stu.Name == "TanRui") )
            {
                stu.Age = 25;
            }
            entities.SaveChanges();

            // 查（使用Linq）
            var query = from s in entities.student
                        select new
                        {
                            Name = s.Name,
                            Age = s.Age
                        };
            foreach(var item in query)
            {
                Console.WriteLine(item);
            }
            Console.ReadKey();
        }
    }
}
```

可通过数据库查看数据变化情况如期望一样。

## Code First（空Code First模型）

创建步骤和`DB First`类似，区别如下：

在选择`实体数据模型内容`时选择`空Code First模型`，直接点击完成即可：

![1555348366529](/images/1555348366529.png)

点击完成后我们发现解决方案中出现了`Model1.cs`文件，同时查看`App.Config`文件发现多了下面内容：

```xml
<connectionStrings>
    <add name="Model1" connectionString="data source=(LocalDb)\MSSQLLocalDB;initial catalog=EF.Model1;integrated security=True;MultipleActiveResultSets=True;App=EntityFramework" providerName="System.Data.SqlClient" />
  </connectionStrings>
```

`VS2019`默认使用`SQLserver数据库文件`格式存储`Code First`生成的数据模型，而此处我们要使用的是`MySQL数据库`，因此需要更改`connectionStrings`参数如下：

```xml
<connectionStrings>
    <add name="Model1" providerName="MySql.Data.MySqlClient" connectionString="server=localhost;userid=tanrui;password=tanrui;database=assignment4;persistsecurityinfo=True" />
  </connectionStrings>
```

配置文件修改好之后，我们还需要修改`Model1.cs`文件，先看自动生成的内容：

```c#
namespace EF
{
    using System;
    using System.Data.Entity;
    using System.Linq;
    [DbConfigurationType(typeof(MySql.Data.Entity.MySqlEFConfiguration))]
    public class Model1 : DbContext
    {
        //您的上下文已配置为从您的应用程序的配置文件(App.config 或 Web.config)
        //使用“Model1”连接字符串。默认情况下，此连接字符串针对您的 LocalDb 实例上的
        //“EF.Model1”数据库。
        // 
        //如果您想要针对其他数据库和/或数据库提供程序，请在应用程序配置文件中修改“Model1”
        //连接字符串。
        public Model1()
            : base("name=Model1")
        {
        }

        //为您要在模型中包含的每种实体类型都添加 DbSet。有关配置和使用 Code First  模型
        //的详细信息，请参阅 http://go.microsoft.com/fwlink/?LinkId=390109。

        public virtual DbSet<MyEntity> MyEntities { get; set; }
    }

    public class MyEntity
    {
        public int Id { get; set; }
        public string Name { get; set; }
    }
}
```

`VS2019`为我们自动生成的`Model1.cs`中已经给出了最基本的`Code First范例`，这里为了简便起见，我们没有新建`MyEntity.cs文件`，而是直接在`Model1.cs`中声明此`Model类`，并在`Model1类`中定义对应的`DbSet对象`。

注意，上述代码中第6行的 `[DbConfigurationType(typeof(MySql.Data.Entity.MySqlEFConfiguration))]`内容需要手动添加，自动生成时不会自动添加。

至此我们假设已经创建了一个简单的逻辑表，现在需要将其同步到数据库中：

![1555348996560](/images/1555348996560.png)

如图，打开`程序包管理器控制台`，输入如下代码：

```
# 为项目启用迁移
PM> Enable-Migrations
# 为当前设计器代码模型生成一次快照，在搭建迁移基架时会使用最近一次的快照，意即更改代码后，需要重新执行此命令
PM> Add-Migration AddMyEntity
# 将更改迁移到数据库
PM> Update-Database -Verbose
```

可能出现的错误：

```
未为提供程序“MySql.Data.MySqlClient”找到任何 MigrationSqlGenerator。请在目标迁移配置类中使用 SetSqlGenerator 方法以注册其他 SQL 生成器。
```

原因：在Model1类定义前未手动添加`[DbConfigurationType(typeof(MySql.Data.Entity.MySqlEFConfiguration))]`。

当上述迁移成功完成后，就可以测试数据库是否真正生效了，测试代码如下：

```C#
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EF
{
    class Program
    {
        static void Main(string[] args)
        {
            Model1 model = new Model1();
            MyEntity entity = new MyEntity
            {
                Id = 1,
                Name = "Tanrui"
            };
            model.MyEntities.Add(entity);
            model.SaveChanges();
            Console.ReadKey();
        }
    }
}
```

运行完之后查看数据库：

![1555349943322](/images/1555349943322.png)

可以看到，`myentities表`已成功添加到数据库中，一行数据也成功插入到表中了。

> Tips: 注意到Code First还有个来自数据库的Code First，它实际上是根据现有的表生成对应的代码设计器，用户需要修改表结构时可通过修改设计器，而后更新迁移再更新到数据库中，个人感觉这和DB First没什么区别，而且不如DB First来的灵活。
>
> ![1555350134801](/images/1555350134801.png)