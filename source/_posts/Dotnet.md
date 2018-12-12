---
title: dotnet基本配置及EFCore连接Mysql
date: 2018-06-26 16:57:08
tags: 
    - Dotnet
    - 数据库

categories: archives
---

## 前奏部分

- 下载并安装[dotnet core](https://www.microsoft.com/net/learn/get-started/)

- 下载并安装[vscode](https://code.visualstudio.com/)（需要把vscode添加到path中）

<!-- more -->

- vscode中搜索并安装C#插件、NuGet Package Manager插件

  > ![image-20180607144445947](http://getme.guitoubing.top/image-20180607144445947.png)

- 新建项目

  > ```shell
  > mkdir dotnet
  > cd dotnet
  > dotnet new mvc
  > code .
  > ```

- commond + shift + p输入nuget add package安装以下依赖包，各个包的Version可在添加时选择

  > ![image-20180607145123118](http://getme.guitoubing.top/image-20180607145223693.png)

  > 添加包时以下代码将自动在dotnet.csproj中添加：
  > ```xml
  > <ItemGroup>
  >     <PackageReference Include="Microsoft.AspNetCore.All" Version="2.0.6"/>
  >     <PackageReference Include="Microsoft.EntityFrameworkCore.Sqlite" Version="2.1.0"/>
  >     <PackageReference Include="Microsoft.EntityFrameworkCore.Tools" Version="2.1.0"/>
  >     <PackageReference Include="Microsoft.EntityFrameworkCore.Sqlite.Design" Version="2.0.0-preview1-final"/>
  >     <PackageReference Include="Microsoft.EntityFrameworkCore.Tools.DotNet" Version="2.1.0-preview1-final"/>
  >     <PackageReference Include="Pomelo.EntityFrameworkCore.MySql" Version="2.1.0-rc1-final"/>
  >     <PackageReference Include="Pomelo.EntityFrameworkCore.MySql.Design" Version="1.1.2"/>
  > </ItemGroup>
  > ```

## Model部分

- 连接数据库创建实体：

  在vscode终端中输入以下命令

  ```
  dotnet ef dbcontext scaffold "server=localhost;userid=user;pwd=password;port=3306;database=university;sslmode=none;" Pomelo.EntityFrameworkCore.MySql -o Models
  ```

- dotnet ef两个问题

  > 问题1：No executable found matching command "dotnet-ef"
  > 解决方法：dotnet.csproj中添加如下行： 
  >
  > ```XML
  >  <ItemGroup>
  > 	<DotNetCliToolReference Include="Microsoft.EntityFrameworkCore.Tools.DotNet" Version="2.1.0-preview1-final"/>
  > </ItemGroup>
  > ```

  > 问题2：Version for package `Microsoft.EntityFrameworkCore.Tools.DotNet` could not be resolved.
  >
  > 原因：上述配置中Version版本与包引用中的版本不一致，修改上述添加代码的Version即可

  此时将会在Models文件夹下创建所有数据库表的实体，同时会创建一个universityContext.cs实体（university为我数据库名称，自行定义），用于对整个数据库的操作。**至此MVC已完成Model部分**。

## Controller及View部分

- 目前项目Models文件夹下已有DBFirst模式生成的实体文件：

  ![image-20180607150002112](http://getme.guitoubing.top/image-20180607150002112.png)

- 我们选择Student的Model创建C-V视图

  > 这里说明一下，MVC模式中Model顾名思义是数据模型、实体，而View和Controller是相互依存的。一般步骤是先创建StudentController.cs文件，定义其中的路由(URL映射，定义了路由之后可以直接通过URL访问该函数)，如本项目中的StudentController.cs中定义的Index：
  >
  > ```C#
  > public IActionResult Index(){
  >         return View(_context.Student.ToList());
  >     }
  > ```
  >
  > 如此定义后，再在Views文件夹下创建对应Controller的文件夹，此处为Student，而在Controller中定义的每一个路由，都要有对应的一个cshtml文件，此处在Student下创建Index.cshtml。简而言之，**View只负责处理布局，Controller只负责处理逻辑。**

  - 创建StudentController.cs

    > ```c#
    > using System;
    > using System.Collections.Generic;
    > using System.Diagnostics;
    > using System.Linq;
    > using System.Threading.Tasks;
    > using Microsoft.AspNetCore.Mvc;
    > using dotnet.Models;
    > using dotnet;
    > 
    > public class StudentController : Controller{
    >     private universityContext _context;
    > 
    >     public StudentController(universityContext context){
    >         _context = context;
    >     }
    >     
    >     public IActionResult Index(){
    >         return View(_context.Student.ToList());
    >     }
    > 
    >     public IActionResult Register(){
    >         return View();
    >     }
    > 
    >     [HttpPost]
    >     [ValidateAntiForgeryToken]
    >     public IActionResult Register(Student student){
    >         if(ModelState.IsValid){
    >             _context.Student.Add(student);
    >             _context.SaveChanges();
    >             return RedirectToAction("Index");
    >         }
    >         return View(student);
    >     }
    > }
    > ```

  - 创建Student文件夹，以及对应路由的cshtml

    > - Index.cshtml
    >
    > ```html
    > @{
    >     ViewData["Title"] = "学生主页";
    > }
    > 
    > <!-- 此处这个model声明不能忘记 -->
    > @model IEnumerable<dotnet.Student>
    > 
    > <table class="table">
    >     <tr>
    >         <th>Id</th>
    >         <th>姓名</th>
    >         <th>系</th>
    >         <th>学分</th>
    >     </tr>
    >     @foreach (var item in Model){
    >         <tr>
    >             <td>
    >                 @Html.DisplayFor(modelItem => item.Id)
    >             </td>
    >             <td>
    >                 @Html.DisplayFor(modelItem => item.Name)
    >             </td>
    >             <td>
    >                 @Html.DisplayFor(modelItem => item.DeptName)
    >             </td>
    >             <td>
    >                 @Html.DisplayFor(modelItem => item.TotCred)
    >             </td>
    >         </tr>
    >     }
    > </table>
    > ```
    >
    > - Register.cshtml
    >
    > ```html
    > @model dotnet.Student
    > 
    > @{
    >     ViewData["Title"] = "注册";
    > }
    > 
    > <form asp-controller="Student" asp-action="Register" method="POST">
    >     <div class="form-group">
    >         <label asp-for="Id" class="col-md-2 control-label">编号：</label>
    >         <div class="col-md-10">
    >             <input class="form-control" asp-for="Id"/>
    >             <span asp-validation-for="Id" class="text-danger"></span>
    >         </div>
    >         <label asp-for="Name" class="col-md-2 control-label">名字：</label>
    >         <div class="col-md-10">
    >             <input class="form-control" asp-for="Name"/>
    >             <span asp-validation-for="Name" class="text-danger"></span>
    >         </div>
    >         <label asp-for="DeptName" class="col-md-2 control-label">系：</label>
    >         <div class="col-md-10">
    >             <input class="form-control" asp-for="DeptName"/>
    >             <span asp-validation-for="DeptName" class="text-danger"></span>
    >         </div>
    >         <label asp-for="TotCred" class="col-md-2 control-label">学分：</label>
    >         <div class="col-md-10">
    >             <input class="form-control" asp-for="TotCred"/>
    >             <span asp-validation-for="TotCred" class="text-danger"></span>
    >         </div>
    >         <div class="col-md-offset-2 col-md-10">
    >             <input type="submit" value="保存" class="btn btn-default"/>
    >         </div>
    >     </div>
    > </form>
    > ```

- 关于抛出以下错误的解决方法

  - 错误：

    > ![image-20180607142732374](http://getme.guitoubing.top/image-20180607142732374.png)

  - 解决方法：

    > ![dotnet配置](http://getme.guitoubing.top/dotnet%E9%85%8D%E7%BD%AE.png)
    >
    > 注意最下面的Tip：由于我们在Startup.cs中已经添加如下代码：
    >
    > ```c#
    > public void ConfigureServices(IServiceCollection services)
    >         {
    >             services.AddDbContext<universityContext>();
    >             services.AddMvc();
    >         }
    > ```
    >
    > 即满足条件“already configured outside of the context in Startup.cs”，因此我们需要将上述图片中的if语句注释掉，如下：
    >
    > ```c#
    > //if (!optionsBuilder.IsConfigured){ 
    > optionsBuilder.UseMySql("server=localhost;userid=root;pwd=tanrui;port=3306;database=university;sslmode=none;");
    > //}
    > ```

  ## 运行项目

  - 调试的方法

    > - vscode下点按“开始调试”
    >
    >   ![image-20180607151747548](http://getme.guitoubing.top/image-20180607151747548.png)
    >
    > - 浏览器将会自动跳转至localhost:5000
    >
    >   ![image-20180607152008469](http://getme.guitoubing.top/image-20180607152008469.png)
    >
    > - 在URL中添加<u>/student</u>或<u>student/index</u>跳转到我们定义的Controller中，一般情况下index路由是可以忽略不写的，此时自动定位到index中：
    >
    >   ![image-20180607152229016](http://getme.guitoubing.top/image-20180607152229016.png)

  - 戳这里下载[Asp.net Core开发实战.pdf](http://getme.guitoubing.top/ASP.NET%20Core%E8%B7%A8%E5%B9%B3%E5%8F%B0%E5%BC%80%E5%8F%91%E4%BB%8E%E5%85%A5%E9%97%A8%E5%88%B0%E5%AE%9E%E6%88%98%20,%E5%BC%A0%E5%89%91%E6%A1%A5%20,2017.04%20,Pg319_14181929.pdf)
