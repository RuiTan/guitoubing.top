---
title: Java - 深入学习Java
date: 2019-03-06 21:01:30
tags:
    - Java
categories:
    - Java
---

## 垃圾回收器—GC

众所周知，Java中的GC负责回收**无用对象占用的内存资源**，但会有特殊情况：假定对象获得了一块"特殊"的内存区域（不是使用new创建的），由于**GC只释放那些经由new分配的内存**，所以GC不知道如何释放该对象的这块"特殊"内存区域。

<!-- more -->

作为应对，Java允许在类中定义`finalize()`方法，它使得在GC回收该对象内存之前先调用`finalize()`方法，并在下一次GC回收发生时，真正回收对象内存。举个例子：某个对象创建时会在屏幕上绘出一些图像，当没有明确将其从屏幕擦除时，图像便可能会永远存在在屏幕上，若在`finalize()`指定擦除的方法，那么在GC回收该对象时将会同时将其图像从屏幕上擦除。

**关键点：**

1. 对象可能不被垃圾回收
2. 垃圾回收并不等于"析构"
3. 垃圾回收只与内存有关

### 避免使用finalize()

> "终结函数无法预料，常常是危险的，总之是多余的。"《Effective Java》，第20页

在Java中一切皆为对象，且创建对象的方法只有new，那么必然存在**通过某种创建对象以外的方式为对象分配了存储空间**。

Native Method(本地方法)是Java中调用非Java代码的方式，此时非Java代码中可能使用了malloc()等分配内存的函数而未使用free()对其释放，此时GC也不会去管这块内存，这就使得需要指定特定的finalize()方法来实现内存的释放。

可见，finalize()不是进行普遍的清理工作的合适方式，因此需要避免使用。

### 终结条件的验证

但是finalize()有个有趣的用法——终结条件。看如下代码：

```java
class Book{
    // Book类，约定其在被回收前必须被签入。
	boolean checkedOut = false;
	Book(boolean checkedOut){
		checkedOut = checkedOut;
	}
	void checkIn(){
		checkedOut = false;
	}
	protected void finalize(){
        // 终结条件，对象未被签入
		if (checkedOut) {
			System.out.println("Error: checked out");
		}
	}
}

public class Main{
	public static void main(String[] args){
        // 创建一个Book对象-novel
		Book novel = new Book(true);
        // 将其签入
		novel.checkIn();
        // 创建一个Book对象，此时该对象未被签入
		new Book(true);
        // 强制执行垃圾回收，此时会先执行finalize
		System.gc();
	}
}

/* 输出：
Error: checked out
*/
```

我们约定所有的Book对象在创建之前都必须被签入，但是在main中，由于疏忽有个新创建的对象未执行签入操作，此时执行垃圾回收，finalize()中的终结条件被激活，把错误反馈给使用者。

> 注意这里使用的System.gc()强制调用垃圾回收器

若没有finalize()将很难实现这种操作。

### GC如何工作

#### 引用计数（未被使用过）

对象创建时便有引用计数，当引用计数变为0时，GC回收该对象内存空间。

缺陷：循环引用不适用，即出现"对象应该被回收，但引用计数不为0"的情况，称作"交互自引用的对象组"。如下所示：

```java
public class Main {
    public static void main(String[] args) {
        // object1指向的对象引用计数器：1
        MyObject object1 = new MyObject();
        // object2指向的对象引用计数器：1
        MyObject object2 = new MyObject();
        // object1指向的对象引用计数器：2
        object1.object = object2;
        // object2指向的对象引用计数器：2
        object2.object = object1;
        // object1指向的对象引用计数器减少为1
        object1 = null;
        // object2指向的对象引用计数器减少为1
        object2 = null;
    }
}
```

我们将`object1`和`object2`赋值为null，意即我们已经不需要该对象，但由于此时对象的引用计数器不为0导致这两个对象永远不会被回收。

#### 停止-复制（stop-and-copy）

遍历所有**引用**找到所有"活"的**对象**，将堆中**所有存活的对象复制到另一个堆中**，没有被复制的便都是垃圾了。

这种策略避免了上述"交互自引用的对象组"无法回收的情况，因为这两个对象不会被看作是存活的对象，即遍历的过程中根本找不到这两个对象（他们不在从GC Root出发连接所有存活结点构成的图中）。

**缺陷：效率低**

1. 复制需要在两个堆之间操作，即需要维护多一倍的空间；
2. 当程序进入稳定状态之后，可能只产生少量垃圾，此时此策略仍然需要进行复制操作，很浪费。

针对第2个情况，有另外一种策略，如下。

#### 标记-清扫（mark-and-sweep）

同样遍历所有**引用**找到所有"活"的**对象**，同时会给该对象进行**标记**，当全部标记工作完成后，开始进行清理工作。没有被标记的对象将会被释放，因此剩下的堆空间是不连续的，此时GC需要使用其他整理的方法来清理内存碎片，称作"标记-整理"。

> 注意，上面两种垃圾回收机制都不是在后台进行的，意即进行垃圾回收时会暂停程序。
>
> 许多文献中有关于"垃圾回收器是低优先级的后台进程"的说法，事实上早期版本的JVM使用这两种策略时并非如此。当可用内存不足时，垃圾回收器会暂停运行程序，而后开展"停止-复制"或"标记-清扫"工作。

"标记-清扫"方式速度相当慢，但是当垃圾很少时，就很快了。

#### 自适应技术

JVM会进行监视，如果所有对象都很稳定，GC的效率降低的话，就切换到"标记-清扫"方式；同样，JVM也会跟踪"标记-清扫"方式，若堆空间出现很多碎片，就会切换回"停止-复制"方式。这就是自适应技术。

这是早期Sun版本的垃圾回收器。

#### 分代垃圾收集（Generational Garbage Collection）

上述无论是"停止-复制"、"标记-清扫"还是"标记-整理"对于日益增长的对象列表，效率会逐渐低下。

![image-20190225225057306](/images/image-20190225225057306.png)

堆被分为三代：

- 年轻代(Young Generation)

  内存空间：**eden:S0:S1 = 8:1:1**

  S0和S1**没有先后顺序**，任何一个都可能是**From survivor space**和**To survivor space**

- 年老代(Old Generation)

  内存空间：年老代:年轻代 ≈ 2:1

- 持久代(Permanent Generation)

  用于存放静态文件，如Java类、方法等。持久代对垃圾回收没有显著影响。有些应用可能动态生成或者调用一些class，例如Hibernate等，在这种时候需要设置一个比较大的持久代空间来存放这些运行过程中新增的类。

下面说明一下对象在分配内存、老化、回收的过程：

1. 首先，任何新对象创建时内存都会分配在年轻代的**eden space**中，**S0**和**S1**两个**幸存者空间(survivor space)**起初都是空的![image-20190225225802212](/images/image-20190225225802212.png)

2. 当eden space满时，会触发第一次**较小的垃圾回收过程(minor garbage collection，minor GC)**![image-20190225230110077](/images/image-20190225230110077.png)

   > 实际上MinorGC不一定要等到eden space满了才触发

3. eden space中所有存活对象(referenced objects)被复制到S0，其余对象(unreferenced objects)被视作垃圾，随eden space一起被回收![image-20190225230647640](/images/image-20190225230647640.png)

4. 当下一次minor GC被触发时，eden space执行与第3点中相同的步骤，不过此时存活对象会被复制到S1，同时S0中的存活对象也会被复制到S1，此时S0和eden space都被回收。注意到此时S1有不同老化程度的对象![image-20190225232204967](/images/image-20190225232204967.png)

5. 再当下一次minor GC被触发时，重复上述操作，幸存者空间变为S0，eden和S1中的存活对象都被复制到S0，同时老化，此时S1和eden space都被回收![image-20190225232407098](/images/image-20190225232407098.png)

6. 当minor GC持续触发到对象老化程度达到一个阈值(此处为8)时，这些对象从年轻代提升到年老代![image-20190225232630665](/images/image-20190225232630665.png)

7. 以上过程涵盖了整个年轻代老化的过程，最终，会在年老代触发**完全的垃圾回收(major gabarge collector, major GC)**，清理并压缩该块内存空间。

   major GC被触发的原因：

   1. 年老代（Tenured）被写满

   2. 持久代（Permanent）被写满

   3. System.gc()被显式调用

   4. 上一次GC之后Heap的各域分配策略动态变化

##### HotSpot JVM的垃圾收集器

**Serial收集器（复制算法)**：新生代单线程收集器，标记和清理都是单线程，优点是简单高效。

**Serial Old收集器(标记-整理算法)**：老年代单线程收集器，Serial收集器的老年代版本。

**ParNew收集器(停止-复制算法)**：新生代收集器，可以认为是Serial收集器的多线程版本,在多核CPU环境下有着比Serial更好的表现。

**Parallel Scavenge收集器(停止-复制算法)**：并行收集器，追求高吞吐量，高效利用CPU。吞吐量一般为99%， 吞吐量= 用户线程时间/(用户线程时间+GC线程时间)。适合后台应用等对交互相应要求不高的场景。

**Parallel Old收集器(停止-复制算法)**：Parallel Scavenge收集器的老年代版本，并行收集器，吞吐量优先

**CMS(Concurrent Mark Sweep)收集器(标记-清扫算法)**：高并发、低停顿，追求最短GC回收停顿时间，cpu占用比较高，响应时间快，停顿时间短，多核cpu 追求高响应时间的选择

【参考：[深入理解JVM(3)——7种垃圾收集器](https://crowhawk.github.io/2017/08/15/jvm_3/)】

## 可变参数列表

Java中的可变参数列表（JSE5之后）的使用与C的使用类似，如下：

```java
public class test{
	public static void main(String[] args){
		Integer a = 1;
		Integer b = 2;
		Integer c = 3;
		Other.main(a, b);
		Other.main(a, b, c);
        Other.main();
        Other.main(new Object[]{a, b});
		Other.main(new Object[]{a, b, c});
	}
}

class Other{
	public static void main(Object... args){
		for (Object s : args){
			System.out.println(s + " ");
		}
	}
}
```

如上所示，当输入不同个数参数时，编译器会自动将其**转换成数组**，当参数本身就是数组时，编译器又**不会进行转换**，直接传递给函数。参数为空时编译器便**直接传递一个空Object数组**。

### 可变参数列表的重载

```java
public class test{
	static void f(Character... args){
		System.out.println("first");
	}
	static void f(String... args){
		System.out.println("second");
	}
	public static void main(String[] args){
		f('a', 'b');
		f("a", "b");
		f();
	}
}
```

如上，函数有`f(Character... args)`和`f(String... args)`两种重载方式，此时`f('a', 'b')`和`f("a", "b")`都可正常调用，但是`f()`会报错，即两种重载都匹配。

此时可通过为其中一个重载函数添加一个非可变参数（可变参数必须位于参数列表最后）。但这样又会产生新的问题，如下：

```java
public class test{
	static void f(float i, Character... args){
		System.out.println("first");
	}
	static void f(Character... args){
		System.out.println("second");
	}
	public static void main(String[] args){
		f(1, 'a');
		f('a', 'b');
	}
}
```

如上，编译器也会报错，`f('a', 'b')`可匹配两个函数，(可能是)因为`char`类型可提升至`float`类型从而匹配第一个重载函数。

此时可为第二个重载函数也添加一个非可变参数，问题可得到解决。

```java
public class test{
	static void f(float i, Character... args){
		System.out.println("first");
	}
	static void f(char i, Character... args){
		System.out.println("second");
	}
	public static void main(String[] args){
		f(1, 'a');
		f('a', 'b');
	}
}
```

这种用法比较奇怪，因此"你应该总是只在重载方法的一个版本上使用可变参数列表，或者压根就不是用它"（《Java编程思想》105页）。

## 内部类

### 内部类对象对外围类对象的访问

当外围类对象创建了一个内部类对象时，此内部类对象必定会秘密地捕获一个指向外围类对象的引用，因此内部类对象可以访问外部类对象的所有成员。

```java
interface Selector {
    boolean end();
    Object current();
    void next();
}

public class Sequence {
    private Object[] items;
    private int next = 0;
    public Sequence(int size) { items = new Object[size]; }
    public void add(Object x){
        if (next < items.length){
            items[next++] = x;
        }
    }
    private class SequenceSelector implements Selector {
        private int i = 0;
        public boolean end() { return i == items.length; }
        public Object current() { return items[i]; }
        public void next() { if(i < items.length) i++; }
    }
    public Selector selector(){
        return new SequenceSelector();
    } 
    public static void main(String[] args){
        Sequence sequence = new Sequence(10);
        for (int i = 0; i < 10; i++){
            sequence.add(Integer.toString(i));
        }
        Selector selector = sequence.selector();
        while(!selector.end()){
            System.out.print(selector.current() + " ");
            selector.next();
        }
    }
}
```

Sequence中的内部类SequenceSelector可以访问Sequence的全部成员，就像SequenceSelector自己拥有这些成员一样。

### 内部类与静态内部类（嵌套类）

#### 创建方法

```java
// 内部类：DotNew.java
public class DotNew {
    class Inner {
        Inner(){
            System.out.println("创建内部类");
        }
    }
    public static void main(String [] args){
        DotNew dn = new DotNew();
        DotNew.Inner dni = dn.new Inner();
    }
}

// 静态内部类：DotNewStatic.java
public class DotNewStatic {
    static class Inner {
        Inner() {
            System.out.println("创建静态内部类");
        }
    }
    public static void main(String[] args){
        DotNewStatic.Inner inner = new DotNewStatic.Inner();
    }
}
```

### 匿名内部类

Java支持**创建一个继承自某基类的匿名类的对象**，通过new表达式返回的引用被**自动向上转型为对基类的引用**。

匿名内部类可以使用默认构造器生成，也可以使用有参数的构造器。

注意，在匿名内部类中若想使用外部定义的对象，该外部对象的参数引用必须是`final`，如下：

```java
// Destination.java
public interface Destination{
    String readLabel();
}

// Parcel9.java
public class Parcel9 {
    public Destination destination(final String dest){// 外部变量dest被引用时需声明为final，否则产生编译时错误
        return new Destination(){
            private String label = dest;
            @Override
            public String readLabel() {
                return label;
            }
        };
    }
    public static void main(String[] args){
        Parcel9 p = new Parcel9();
        Destination d = p.destination("Tasmania");
        System.out.println(d.readLabel());
    }
}
```

> 但是我使用的Java 10中，当dest不声明为final时也不会报错，虽然不会报错，但是当更改dest引用时会报前面所述的编译时错误（Local variable dest defined in an enclosing scope must be final or effectively final）。

> **为什么匿名内部类访问外部变量必须是final的？**
>
> 1. 为了避免**外部方法修改引用导致内部类得到的引用值不一致**和**内部类修改引用而导致外部方法的参数值在修改前和修改后不一致**
>
> 2. 保证回调函数回调时可访问到变量（**待研究**）
>
> 3. 反编译查看其实现细节：
>
>    ```java
>    // 源代码
>    public interface MyInterface {
>        void doSomething();
>    }
>    public class TryUsingAnonymousClass {
>        public void useMyInterface() {
>            final Integer number = 123;
>            System.out.println(number);
>    
>            MyInterface myInterface = new MyInterface() {
>                @Override
>                public void doSomething() {
>                    System.out.println(number);
>                }
>            };
>            myInterface.doSomething();
>    
>            System.out.println(number);
>        }
>    }
>    
>    // 反编译结果
>    class TryUsingAnonymousClass$1
>            implements MyInterface {
>        private final TryUsingAnonymousClass this$0;
>        private final Integer paramInteger;
>    
>        TryUsingAnonymousClass$1(TryUsingAnonymousClass this$0, Integer paramInteger) {
>            this.this$0 = this$0;
>            this.paramInteger = paramInteger;
>        }
>    
>        public void doSomething() {
>            System.out.println(this.paramInteger);
>        }
>    }
>    ```
>
>    注意到，number在实际使用时是作为构造函数的参数传入到匿名内部类的，也就是说匿名类内部在使用外部变量时**实际上是做了个"拷贝"**或者说**"赋值"**。若可以更改，则会造成数据不一致。

## RTTI

RTTI(Run-Time Type Identifier)是Java能在运行时自动识别出某个类型的保证（RTTI在Java运行时维护类的相关信息），是**多态的基础**，由**Class类实现**。

### Class对象

每当编写并且编译一个类时，在与类同名的`.class`文件中会自动产生一个`Class对象`。实现此过程的JVM子系统被称作**类加载器**。

Class对象仅在需要的时候才被加载，也就是所有的类都是**只在对其第一次使用时**，动态加载到JVM中的。所谓第一次使用指的是**对类的非常量静态域的第一次引用。**

- 要注意，**类的构造器**是**隐性非常量静态域**，所以使用new操作符生成对象也是产生这样的Class类引用。

- 与此同时，还可以使用`Class.forName(类名)`产生Class对象的引用，告诉JVM去加载这个类。当JVM未找到这个类，会抛出异常`ClassNotFoundException`。比如在JDBC连接数据库时常常用到的`Class.forName("com.mysql.jdbc.Driver")`，就是告诉JVM去加载MySQL驱动。

- 当已经拥有某个类型的对象（实例）时，可通过调用`getClass()`方法来获取该类型的Class引用。

- 另一种方法，使用**类字面变量**。通过使用`类名.class`可获取此类的Class对象的引用，但是注意，此时**此Class对象还未被初始化**，还需要等到上述的`对类的非常量静态域的第一次引用`这一操作执行时才被初始化。

  > 使用.class方法获取Class对象引用实际包含三个步骤：
  >
  > 1. **加载**：类加载器创建Class对象
  > 2. **链接**
  > 3. **初始化**：如果该类具有超类，则对其初始化，执行静态初始化器和静态初始化块
  >
  > 考虑如下代码：
  >
  > ```java
  > import java.util.Random;
  > 
  > class Initable {
  >     static final int staticFinal = 1;
  >     static final int staticFinal2 = ClassInitialization.rand.nextInt(1000);
  >     static {
  >         System.out.println("Initializing Initable");
  >     }
  > }
  > 
  > class Initable2 {
  >     static int staticNonFinal = 2;
  >     static {
  >         System.out.println("Initializing Initable2");
  >     }
  > }
  > 
  > class Initable3 {
  >     static int staticNonFinal = 3;
  >     static {
  >         System.out.println("Initializing Initable3");
  >     }
  > }
  > 
  > public class ClassInitialization {
  >     public static Random rand = new Random(47);
  >     public static void main(String[] args) throws ClassNotFoundException {
  >         // 创建Initable的Class对象的引用，Class对象未初始化
  >         Class initable = Initable.class;
  >         // 仍然未初始化，因Initable.staticFinal是常数
  >         System.out.println(Initable.staticFinal);
  >         // 触发了Initable的Class对象的初始化
  >         System.out.println(Initable.staticFinal2);
  >         // 触发了Initable2的Class对象的初始化
  >         System.out.println(Initable2.staticNonFinal);
  >         // 创建Initable3的Class对象的引用，同时会初始化此Class对象
  >         Class initable3 = Class.forName("Initable3");
  >         // 此时已初始化，无需再次初始化
  >         System.out.println(Initable3.staticNonFinal);
  >     }
  > }
  > ```

另外，当我拥有某个Class对象c的时候，我虽然**不知道它确切类型**，但是可以使用`c.newInstance()`来正确地获取c代表的类型的实例。**但是此方法要求对应的类**。

### 泛化的Class对象引用

Class对象可以通过`Class<Type>`的方法产生特定类型的类引用，创建了**使用类型限定后**的Class对象引用**不能再赋值给除本身和子类的其他的Class对象**。

> 注意这里的子类指的是Class对象的继承关系，而不是类本身的继承关系，如`Integer`继承自`Number`，而`Integer Class对象`却不是`Number Class对象`的子类。

使用通配符`Class<?>`优于平凡的`Class`（实际上是等价的），而且会免除编译器警告，看图：![image-20190308105552914](/images/image-20190308105552914.png)

一种更好的用法，`Class<? extends Type>`，这种类型限定比直接`Class<Type>`好的地方在于他产生的Class对象引用**可赋值给Type本身及子类的Class对象**，这种继承关系**是Type所属的继承关系**而**不是对应的Class对象的继承关系**。

### 转型语法（不常用）

```java
class Building{}
class House extends Building {}

public class ClassCasts {
    public static void main(String[] args){
        Building b = new Building();
        Class<House> houseType = House.class;
        House h = houseType.cast(b);
        h = (House) b;
    }
}
```

如上，使用`houseType.cast(b)`和`(House) b`效果一样，但是执行的工作却不同，具体内部实现尚未学习到。

### 动态的类型检测

#### obj instanceof ClassType

返回一个布尔值，告诉我们某个对象是不是某个特定类型的实例。

#### ClassType.isInstance()

返回一个布尔值，告诉我们某个对象的类型是不是可以被强转为某个特定类型。

#### 区别

区别主要是后者与前者动态等价，看代码：

```java
class Father{}
class Son extends Father{}

public class DynamicEqual {
    public static void main(String[] args){
        Father father = new Father();
        Son son = new Son();
        // instanceof关键词后面必须跟类型的名称，意即其必须首先知道类型名称
        // if (son instanceof father.getClass()){
        //     ...
        // }
        // isInstance()方法是类对象的方法，任何一种类型的类对象的引用都可调用该方法，简言之，其前面的Class类对象是可动态的。
        if (father.getClass().isInstance(son)){
            System.out.println("isInstance is Dynamic");
        }
    }
}
```

#### 优点

`isInstance()`的存在可以替代`instanceof`，而且可使得代码更简洁。比如说有多个类{A1,A2,A3,…}都继承自A，现有一个A对象实例，要判断其为子类中的哪一个从而产生不同响应时：

- 使用`instanceof`时可能需要使用`switch-case`语句；当需要添加一个子类时，需要修改`switch-case`内部代码。
- 而使用`isInstance()`时，可创建一个列表存储所有的子类类型，主程序只需要使用一个循环检测该实例即可；当需要添加一个子类时，只需要修改子类类型列表而不用修改程序代码。

### 反射机制

#### 反射与RTTI的区别

- RTTI：编译器在编译时打开和检查`.class`文件（获取类的Class类对象信息）
- 反射：JVM在运行时打开和检查`.class`文件（编译时可能没有此文件，但是在运行时必须在本地机器或者网络上获取`.class`文件）

#### 类方法提取器

通过Class对象引用：调用`getMethods()`方法获取该类及其父类的方法列表，调用`getConstructors()`方法获取该类的构造方法列表。要注意能获得的方法与该类的访问权限有关，一个**非public类的非public方法是无法被获取的**。

#### 接口与类型信息

interface关键字的一种重要目标就是允许程序员**隔离构件，进而降低耦合性**。

#### 包权限安全吗？

直接看例子：

```java
// A.java
public interface A{
    void f();
}

// HiddenC.java
class C implements A{

    @Override
    public void f() {
        System.out.println("public C.f()");
    }
    public void g(){
        System.out.println("public C.g()");
    }
    void u(){
        System.out.println("package C.u()");
    }
    protected void v(){
        System.out.println("protected C.v()");
    }
    private void w(){
        System.out.println("private C.w()");
    }
}
public class HiddenC {
    public static A makeA(){
        return new C();
    }
}

// HiddenImplementation.java
import java.lang.reflect.Method;
public class HiddenImplementation {
    public static void main(String[] args) throws Exception {
        A a = HiddenC.makeA();
        a.f();
        System.out.println(a.getClass().getName());
        callHiddenMethod(a, "g");
        callHiddenMethod(a, "u");
        callHiddenMethod(a, "v");
        callHiddenMethod(a, "w");
    }
    static void callHiddenMethod(Object a, String methodName) throws Exception{
        // 获取a中的方法
        Method g = a.getClass().getDeclaredMethod(methodName);
        // 修改该方法的权限
        g.setAccessible(true);
        // 调用该方法
        g.invoke(a);
    }
}
/* output
public C.f()
C
public C.g()
package C.u()
protected C.v()
private C.w()
*///:)
```

- 当我知道一个类中有哪些方法时，哪怕是private方法仍然可以在使用`setAccessble(true)`后被调用。
- 只发布`.class`文件也是没办法避免此问题，`javap -private`命令可以反编译`.class`文件，`-private`参数约定显示所有的成员
- 同样，内部类和匿名内部类也是没办法避免此情况

## 泛型

### 指定类型有保证吗？

> 在泛型代码内部，无法获得任何有关泛型参数类型的信息。

例如对于`ArrayList<String>`和`ArrayList<Integer>`，二者的实例调用`.getClass()`获取的Class对象时相同的，如下：

```java
import java.util.ArrayList;

public class Erase{
    public static void main(String[] args){
        Class<?> s = new ArrayList<String>().getClass();
        Class<?> i = new ArrayList<Integer>().getClass();
        System.out.println(s == i);
        System.out.println(s.getName());
        System.out.println(s.getTypeParameters());
    }
}
/* Output:
true
java.util.ArrayList
[Ljava.lang.reflect.TypeVariable;@68f7aae2
*///
```

但是，如果在一个`ArrayList<String>`类型的实例中添加`Integer`会报编译期错误，这个很容易理解（静态类型检查）。但是上述的Class对象相同有给了我们可乘之机：

```java
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;

class Apple{
    @Override
    public String toString(){
        return "This is an apple";
    }
}

public class ReflectAdd{
    public static void main(String[] args) throws Exception{
        ArrayList<String> strings = new ArrayList<>();
        Class<?> s = strings.getClass();
        Method method = s.getMethod("add", Object.class);
        method.invoke(strings, 1);
        method.invoke(strings, "2");
        method.invoke(strings, 3);
        method.invoke(strings, new Apple());
        System.out.println(Arrays.toString(strings.toArray()));
        for (Object o : strings){
            System.out.println(o.getClass());
        }
    }
}
/* Output:
[1, 2, 3, This is an apple]
class java.lang.Integer
class java.lang.String
class java.lang.Integer
class Apple
*///
```

我们可以看到上述代码使用反射机制成功的在`ArrayList<String>`里面添加了`Integer`，原因在于`ArrayList`的泛型实现`ArrayList<E>`使其被擦除为`ArrayList<Object>`，从而通过反射机制找到其`add(E e)`方法时，实际上是`add(Object o)`，而我们代码中的`Method method = s.getMethod("add", Object.class);`恰好可以找到包含这样一个参数列表的add方法，后面也就理所当然的可以添加任意类型(甚至是自定义的Apple类)的实例了。

### 与C++的区别

**C++:**

```cpp
#include <iostream>
using namespace std;

template<class T> class Manipulator {
    T obj;
public :
    Manipulator(T x) { obj = x; }
    void manipulate() { obj.f(); }
    void manipulate2() { obj.noF(); }
};

class HasF {
public:
    void f(){
        cout << "HasF()::f()" << endl;
    }
};

class DontHaveF{
public:
    void noF(){
        cout << "Don't have f()" << endl;
    }
};

int main(){
    HasF hf;
    Manipulator<HasF> manipulator(hf);
    manipulator.manipulate();
    // manipulator.manipulate2();  无法编译
    DontHaveF dhf;
    Manipulator<DontHaveF> manipulator2(dhf);
    // manipulator2.manipulate();  无法编译
    manipulator2.manipulate2();
}
/* Output:
HasF()::f()
Don't have f()
*///
```

模板类`Manipulator`在编译时期便可以检测到函数`f()`、`noF()`是在类型参数<T>中存在的，这是在编译器看到声明`Manipulator<HasF> manipulator(hf)`和`Manipulator<DontHaveF> manipulator2(dhf)`所产生的结果。

然而Java中却无法实现这样的操作：

**Java:**

```java
// HasF.java
public class HasF{
    public void f(){
        System.out.println("HasF.f();");
    }
}

// Manipulation.java
import java.lang.reflect.Method;

class Manipulator<T> {
    private T obj;
    public Manipulator(T x) {
        obj = x;
    }
    public void manipulate(){
        obj.f() // 会报编译错误
    }
}

public class Manipulation{
    public static void main(String[] args){
        HasF hf = new HasF();
        Manipulator<HasF> manipulation = new Manipulator<HasF>(hf);
        manipulation.manipulate();
    }
}
```

由于Java在编译过程中，`Manipulator<T>`是无法确定其类型参数，只知道他是一个`Object实例`，因此obj**只能调用Object基类所有的公开方法**。若想实现C++的操作有两种办法(目前我已知的只有这两种)。

- 为`T`限定参数类型（给定边界），即声明时指定其所继承的基类：

  ```java
  class Manipulator<T extends HasF>{
      ...
  }
  ```

- 使用反射机制调用` f()`

  ```java
  import java.lang.reflect.Method;
  
  class Manipulator<T> {
      private T obj;
      public Manipulator(T x) {
          obj = x;
      }
      public void manipulate() throws Exception{
          Class<?> oc = obj.getClass();
          Method method = oc.getMethod("f");
          method.invoke(obj);
      }
  }
  
  public class Manipulation{
      public static void main(String[] args) throws Exception{
          HasF hf = new HasF();
          Manipulator<HasF> manipulation = new Manipulator<HasF>(hf);
          manipulation.manipulate();
      }
  }
  /* Output:
  HasF.f();
  *///
  ```

### 擦除带来的问题

> 擦除的主要正当理由是从非泛化代码到繁华代码的转变过程，以及在不破坏现有类库的情况下，将泛型融入Java语言。

泛型**不能用于显式地引用运行时类型的操作之中**，例如转型、instanceof、new表达式，因为在静态类型检测之后，泛型就已经被擦除了。

也就是说，需要时刻提醒自己，我只是**看起来好像拥有**有关参数的类型信息而已。实际上，**它只是一个Object！**

### 边界

既然编译器会`擦除`类型信息，那么擦除发生的地点是在哪儿呢？便是所谓的`边界`：对象进入和离开方法的地点，也就是编译器在执行类型检查并插入转型代码的地点。

### 通配符

```java
import java.util.Arrays;
import java.util.List;

class Fruit{}
class Apple extends Fruit{}
class Jonathan extends Apple{}
class Orange extends Fruit{}

public class CompilerIntelligence{
    public static void main(String[] args){
        List<? extends Fruit> flist = Arrays.asList(new Apple());
        Apple a = (Apple) flist.get(0);
        // Orange o = (Orange) flist.get(0); 运行时错误
        // flist.add(new Fruit());  编译错误
        // flist.add(new Apple());  编译错误
        System.out.println(flist.contains(a));
        System.out.println(flist.contains(new Apple()));
        System.out.println(flist.indexOf(new Apple()));
    }
}
/*
true
false
-1
*///
```

对于使用了通配符的`List<? extends Fruit> flist`来说，其需要用到类型参数的方法例如`add()`参数也变成了`<? extends Fruit>`，然而编译器并不能知道这里需要哪一个具体的子类型，于是**编译器拒绝了所有对参数列表中涉及到了通配符的方法的调用，除了构造器。**

## 容器

完整的容器分类法：

![image-20190312200304819](/images/image-20190312200304819.png)

### HashMap

`HashMap`采用了链地址法，也就是**数组+链表**的方式。主干是一个`Entry`数组，链表是为了解决哈希冲突而存在的。`HashMap`中的链表越少，性能越好。

#### Entry数组长度为2的次幂

- 由于在计算`key`的`插入位置`时用到了`hash & (length-1)`，`hash`是`key`计算出来的哈希值，想象一下当`length`不为2的次幂时，`length-1`的二进制必然有`0位`，那么意味着该位为`0`的位置永远不可能被当做`插入位置`，造成了严重的空间浪费。
- 由于刚才的原因，数组可以使用的位置比数组长度小了很多，意味着进一步增加了碰撞的几率，意即`equal()`操作多了起来，效率也就慢了。

#### resize

`HashMap`当`Entry`数组元素超过`数组大小*loadFactor`时，就会进行数组扩容。`loadFactor`默认值为0.75。此时`Entry`数组大小会扩大一倍，保证了2的次幂大小。

扩容的时候所有的`key`需要重新计算哈希值。

#### JDK1.8优化

由于1.8之前的`HashMap`在`hash`冲突很大时，遍历链表将会效率很低，于是1.8中采用了红黑树部分代替链表，当链表长度到达阈值时，就会改用红黑树存储。

### HashTable

`HashTable`在结构上与`HashMap`基本相同，下面总结其不同点：

- `HashMap`可有`null key`，`HashTable`获取`null key`会报空指针异常
- `HashTable`有`synchronized`方法同步，线程安全；`HashMap`线程不安全
- `Hash`值计算方法不同
- `HashTable`初始大小为`11`，扩容机制为`2*old+1`；`HashMap`初始大小为`16`，扩容机制为`2*old`

### ConcurrentHashMap

`JDK1.7`版本中的`ConcurrentHashMap`比`HashMap`多了一层`Segment`，其中`Segment`继承于`ReentrantLock`：**一次`put`操作会调用`scanAndLockForPut()`方法自旋获取锁**；**而一次`get`操作则不需要加锁，`value`用`volatile`关键词修饰的，保证了内存可见性，每次获取的必定是新值，由于不用加锁，所以很高效**。

`JDK1.8`版本移除了`segment`，有一个`Node`数组相当于`HashMap`中的`Entry`数组。同时采用了`CAS+synchronized关键字`进行`put`操作。`put`操作步骤如下：

- 根据`key`计算出`hashcode`；
- 判断是否需要进行初始化；
- `f` 即为当前 `key` 定位出的 `Node`，如果为空表示当前位置可以写入数据，利用 `CAS` 尝试写入，失败则自旋保证成功；
- 判断是否需要进行扩容；
- 如果都不满足，则利用 `synchronized` 锁写入数据；
- 如果数量大于 `TREEIFY_THRESHOLD` 则要转换为红黑树。

## 线程

### Brian Goetz的线程同步规则

> 如果你正在写一个变量，他可能接下来将被另一个线程读取，或者正在读取一个上一次已经被另一个线程写过的变量，那么必须使用同步，并且，**读写线程**都必须用相同的监视器锁同步。

### Executor

Executor用来管理Thread对象，简化了并发编程，允许管理异步任务的执行，而无须显式管理线程的声明周期。

```java
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class CachedThreadPool {
    public static void main(String[] args){
        ExecutorService exec = Executors.newCachedThreadPool();
        for (int i = 0; i < 5; i++){
            exec.execute(new LiftOff());
        }
        exec.shutdown();
    }
}
/* Output:
#4(9).#2(9).#1(9).#2(8).#3(9).#4(8).#0(9).#1(8).#2(7).#3(8).#4(7).#0(8).#1(7).#2(6).#3(7).#4(6).#0(7).#1(6).#2(5).#3(6).#4(5).#0(6).#1(5).#2(4).#3(5).#4(4).#0(5).#1(4).#2(3).#3(4).#4(3).#0(4).#1(3).#2(2).#3(3).#4(2).#0(3).#1(2).#2(1).#3(2).#4(1).#0(2).#1(1).#2(LiftOff!).#3(1).#4(LiftOff!).#0(1).#1(LiftOff!).#0(LiftOff!).#3(LiftOff!).
*///
```

### 线程池

线程池的作用是**限制系统中执行线程的数量**，根据系统情况可以**自动或手动**设置线程数量，达到最佳运行效果。线程池中的线程若出现异常，会自动补充一个新线程以代替。

- `newSingleThreadExecutor()`：创建一个单线程的线程池，所有的任务在等待队列中等待该线程。
- `newFixedThreadPool()`：创建固定大小的线程池。
- `newCachedThreadPool()`：创建一个可缓存的线程池。会根据任务数量自动添加和回收线程，线程池的大小依赖于JVM能够创建的最大线程大小。
- `newScheduledThreadPool()`：创建一个大小无限的线程池，此线程支持定时以及周期性执行任务的需求。

### 任务的返回值

通常实现`Runnable`接口的类是没有返回值的，要想任务在完成时返回一个值可实现`Callable<T>`接口，其泛型类型参数表示方法`call()`的返回值，并且需要使用`ExecutorService.submit()`方法调用他。

```java
import java.util.ArrayList;
import java.util.concurrent.*;

class TaskWithResult implements Callable<String> {
    private int id;

    public TaskWithResult(int id) {
        this.id = id;
    }

    public String call() {
        return "result of TaskWithResult" + id;
    }
}

public class CallableDemo {
    public static void main(String[] args) {
        ExecutorService exec = Executors.newCachedThreadPool();
        ArrayList<Future<String>> results = new ArrayList<>();
        for (int i = 0; i < 10; i++) {
            results.add(exec.submit(new TaskWithResult(i)));
        }
        for (Future<String> fs : results) {
            try {
                System.out.println(fs.get());
            } catch (InterruptedException | ExecutionException e) {
                System.err.println(e);
            } finally {
                exec.shutdown();
            }
        }
    }
}
/* Output：
result of TaskWithResult0
result of TaskWithResult1
result of TaskWithResult2
result of TaskWithResult3
result of TaskWithResult4
result of TaskWithResult5
result of TaskWithResult6
result of TaskWithResult7
result of TaskWithResult8
result of TaskWithResult9
*///
```

`ExecutorService`对象的`submit()`方法会返回一个`Future<T>`对象，泛型类型参数即是实现`Callable<T>`的类型参数。`get()`方法会返回结果，若任务未完成，`get()`会阻塞。

### 优先级

优先权不会导致死锁，优先级较低的线程仅仅是执行的频率较低。

但是注意优先级高的线程也有几率比优先级底的线程执行的少。

优先级是否起作用也与操作系统及虚拟机版本相关联，会随着不同的线程调度器而产生不同的含义。

### Thread.yield()可靠吗？

`Thread.yield()`源码中提及了该方法的效果：**当前线程会给线程调度器一个暗示，说明我愿意让出当前资源供你调度，但是线程调度器可自由的选择是否忽略其暗示。**意即此处的`让步`只是一厢情愿，发出让步的线程同样可以继续执行。

### 后台线程

后台线程**并不属于程序中不可或缺的部分**。当所有的非后台线程结束时，程序也就终止了，同时会杀死进程中的所有后台线程。

执行`main()`就是一个非后台线程，当`main()`没有执行结束时，程序就不会终止。

后台线程创建的线程也将是后台线程。

同时要注意在后台线程的`run()`方法中若有`finally`子句，其中的语句也不一定会执行。因为随着非后台线程的结束，后台线程会突然终止。

### Thread还是Runnable

创建多线程任务可以继承`Thread`类重写其`run()`方法，也可以实现`Runnable`接口实现其`run()`方法。

实际应用中，`Runnable`还是比较有优势的：

- 避免了由于Java的单继承体系带来的局限（实际上继承Thread也是可以避免，使用内部类）
- 多个线程区处理同一资源，而非独立处理（这句话有问题）

注意，一开始在理解这里的时候我出现了误解，什么叫**处理同一资源**，意思指的是Thread类无法达到资源共享的目的，而Runnable可以。但是在使用线程池的时候，Thread又可以了**(待确认)**，如下：

```java
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

class TestThread extends Thread {
    private int val = 10;
    public void run(){
        while(true){
            System.out.println(Thread.currentThread() + "-- val: " + val--);
            Thread.yield();
            if  (val <= 0)
                return;
        }
    }
}

class TestRunnable implements Runnable {
    private int val = 10;
    @Override
    public void run() {
        while(true){
            System.out.println(Thread.currentThread() + "-- val: " + val--);
            Thread.yield();
            if (val <= 0)
                return;
        }
    }    
}

public class TestRunnableAndThread {
    public static void main(String[] args){
        Runnable runnable = new TestRunnable();
        Thread thread = new TestThread();
        // a.只有1个线程处理一个数据
        thread.start();
        thread.start();
        thread.start();
        thread.start();
        thread.start();
        // b.5个不同线程处理不同数据
        new TestThread().start();
        new TestThread().start();
        new TestThread().start();
        new TestThread().start();
        new TestThread().start();
        // c.5个不同线程处理相同数据
        new Thread(runnable).start();
        new Thread(runnable).start();
        new Thread(runnable).start();
        new Thread(runnable).start();
        new Thread(runnable).start();
        // d.5个不同线程处理相同数据
        ExecutorService execRun = Executors.newCachedThreadPool();
        for (int i = 0; i < 5; i++)
            execRun.execute(runnable);
        // e.5个不同线程处理5个不同数据
        ExecutorService execRun2 = Executors.newCachedThreadPool();
        for (int i = 0; i < 5; i++)
            execRun2.execute(new TestRunnable());
        // f.5个不同线程处理相同数据
        ExecutorService execThread = Executors.newCachedThreadPool();
        for (int i = 0; i < 5; i++){
            execThread.execute(thread);
        }
        // g.5个不同线程处理5个不同数据
        ExecutorService execThread2 = Executors.newCachedThreadPool();
        for (int i = 0; i < 5; i++)
            execThread2.execute(new TestThread());
        // i.5个不同线程处理相同数据
        ExecutorService execThread2 = Executors.newCachedThreadPool();
        for (int i = 0; i < 5; i++)
            execThread2.execute(new Thread(runnable));

        execRun.shutdown();
        execRun2.shutdown();
        execThread.shutdown();
        execThread2.shutdown();
    }
}
```

其中：**c、d、i**实际上是相同的，**b、g**是相同的，而**a**和**f**看起来相同，但是实际作用却差别很大，待研究。

实际上，**a**是错误的用法，**b**、**c**基本上不用，而且，注意当需要共享数据的时候，通常不会在类中定义共享变量，而需要一个**线程安全的外部对象**。

### 共享资源

#### Synchronized

冲突是多线程问题必须解决的任务，Java使用`synchronized`关键字标识访问共享资源的方法，JVM负责跟踪对象被加锁的次数，注意，当对象被解锁（完全释放时）其加锁计数为0，显然此时所有任务都有几率向其加锁，当**某一个任务第一次给该对象加锁时，计数变为1**，此后**只有这个相同的任务能继续给该对象加锁**，计数会递增；**每当离开一个synchronized方法时，计数递减**，直到计数变为0时，对象被解锁。要注意，**每个访问该临界资源的方法都必须被同步**，否则就不会正确地工作。

通常`synchronized`关键字标识方法时，是在`this`上面同步，也可在方法中使用`synchronized(synObject){}`域，**以在特定的对象上同步**，因此不同对象上的锁是相互无关的。

#### Lock

Lock对象必须被显式地创建、锁定和释放。

```java
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class MutexEvenGenerator {
    private int currentEvenValue = 0;
    // 显式声明
    private Lock lock = new ReentrantLock();
    public int next() {
        // lock()方法创建临界资源
        lock.lock();
        try {
            ++currentEvenValue;
            Thread.yield();
            ++currentEvenValue;
            // return语句必须出现在try子句中
            return currentEvenValue;
        }finally {
            // unlock()方法完成清理工作
            lock.unlock();
        }
    }
}
```

与`synchronize`相比，显式的`Lock`优点在于可以使用`finally子句`将系统维护在正常的状态，而在使用`synchronize`关键字时，某些事物失败了就会抛出异常。

```java
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;
public class AttemptLocking{
    private ReentrantLock lock = new ReentrantLock();
    public void untimed() {
        boolean captured = lock.tryLock();
        try {
            System.out.println("untimed - tryLock(): " + captured);
            System.out.println("untimed - isHeldByCurrentThread(): " + lock.isHeldByCurrentThread());
        } finally {
            if (captured) 
                lock.unlock();
        }
    }
    public void timed() {
        boolean captured = false;
        try {
            captured = lock.tryLock(2, TimeUnit.SECONDS);
        } catch(InterruptedException e){
            throw new RuntimeException(e);
        }
        try {
            System.out.println("timed - tryLock(2, TimeUnit.SECONDS): " + captured);
            System.out.println("timed - isHeldByCurrentThread(): " + lock.isHeldByCurrentThread());
        } finally {
            if (captured)
                lock.unlock();
        }
    }
    public static void main(String[] args){
        final AttemptLocking al = new AttemptLocking();
        al.untimed();
        al.timed();
        // 匿名内部类创建单独的Thread来获取锁，而未释放
        new Thread(){
            {setDaemon(true);}
            public void run(){
                al.lock.lock();
                System.out.println("acquired");
                System.out.println("main - isHeldByCurrentThread(): " + al.lock.isHeldByCurrentThread());
            }
        }.start();
        Thread.yield();
        al.untimed();
        al.timed();
    }
}
/* Output:
untimed - tryLock(): true
untimed - isHeldByCurrentThread(): true
timed - tryLock(2, TimeUnit.SECONDS): true
timed - isHeldByCurrentThread(): true
acquired
main - isHeldByCurrentThread(): true
untimed - tryLock(): false
untimed - isHeldByCurrentThread(): false
timed - tryLock(2, TimeUnit.SECONDS): false
timed - isHeldByCurrentThread(): false
*///
```

看代码就很容易理解了。

#### 原子性与易变性

原子操作**有可能无需同步机制**，因为操作是不可分的，一次操作进行的时候不会有其他操作的介入，但是实现原子操作是很难的，或者说原子操作是较少存在的。同时，即使操作是原子性的，操作的修改也可能暂时性地存储在本地处理器的缓存中，对于其他任务有可能是**不可视的**，因此不同的任务对应用状态有不同的视图。

**volatile关键字**确保了前面提及的可视性，以及当一个域被声明为volatile时，那么**只要对这个域产生了写操作，所有的读操作都可以看到这个修改**。即使使用了本地缓存，volatile域的修改也会被立即写入到主存中。

所以**非volatile域**上的原子操作未刷新到主存中去，因此其他读操作未必会看到新值。

因此多个任务在同时访问某个域时，**要么使用volatile关键字限定，要么经由同步机制访问**，以保证一致性。

```java
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AtomicityTest implements Runnable {
    private int i = 0;
    public int getValue() { 
        return i;
    }
    private void evenIncrement() {
            i++; i++;
    }
    @Override
    public void run() {
        while (true)
            evenIncrement();
    }
    public static void main(String[] args){
        ExecutorService exec = Executors.newCachedThreadPool();
        AtomicityTest at = new AtomicityTest();
        exec.execute(at);
        while (true){
            int val = at.getValue();
            if (val%2 != 0){
                System.out.println(val);
                System.exit(0);
            }
        }
    }
}
```

看上面这个例子，程序找到奇数时便终止，理想状态下，通过`evenIncrement()`加2，`i`应该始终为偶数，但是由于缺少同步机制，可能导致不稳定的中间状态被读取即获取到奇数，同时`i`也不是`volatile`的，因此还存在可视性问题（当然，这里仅仅使用`volatile`限定`i`是不够的，因为`i++`操作不是原子性的）。下面使用`Lock`显式加锁以实现同步：

```java
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class AtomicityTest implements Runnable {
    private int i = 0;
    private Lock lock = new ReentrantLock();
    public int getValue() { 
        try {
            lock.lock();
            return i; 
        } finally {
            lock.unlock();
        }
    }
    private void evenIncrement() { 
        try {
            lock.lock();
            i++; i++; 
        } finally {
            lock.unlock();
        }
    }
    @Override
    public void run() {
        while (true)
            evenIncrement();
    }
    public static void main(String[] args){
        ExecutorService exec = Executors.newCachedThreadPool();
        AtomicityTest at = new AtomicityTest();
        exec.execute(at);
        while (true){
            int val = at.getValue();
            System.out.println(val);
            if (val%2 != 0){
                System.out.println(val);
                System.exit(0);
            }
        }
    }
}
```

### 原子类

上面说到**原子操作是较少的**，而`JSE5`引入了`AtomicInteger`、`AtomicLong`、`AtomicReference`等特殊的原子性变量类，这些类的一些方法在某些机器上可以是原子的。通常用在性能调优方面。

### ReetrantLock

ReentrantLock是一个**可重入**的**互斥锁**，又被称为"**独占锁**"。

> **可重入锁**指的是某个线程获取锁之后，在执行相关的代码块时可继续调用加了同样的锁的方法，理解为嵌套锁。反之，不可重入锁称作自旋锁。
>
> **独占锁**指的是同一时间点锁只能被一个线程获取。

同时ReentrantLock也分为**公平锁**和**非公平锁**，它们的区别体现在获取锁的机制是否公平。公平锁通过一个FIFO等待队列管理等待获取该锁的所有进程，而非公平锁不管是否在队列中，都直接获取该锁。

### ReentrantReedWriteLock

顾名思义，ReentrantReadWriteLock维护了**读取锁**和**写入锁**。

读取锁用于只读操作，是**共享锁**，能被多个线程获取；

写入锁用于写入操作，是**独占锁**，只能被一个线程获取。

### 线程状态

- 新建（new）
- 就绪（Runnable）
- 阻塞（Blocked）
  - 调用`sleep(milliseconds)`方法使任务休眠
  - 调用`wait()`方法挂起
  - 等待输入输出完成
  - 获取锁失败
- 死亡（Dead）

### 线程协作

#### wait()

与`sleep()`和`yield()`不同，调用`wait()`时**需要释放当前线程获取的锁**，由于某个条件不成立使得当前线程进入阻塞状态，直到其他修改使得此条件发生了变化调用了`notifyAll()`方法时，线程被唤醒。

但是要注意，使用`wait()`的时候需要用`while`循环包围：

- 为了检查线程是否被意外唤醒

#### notifyAll()

`notifyAll()`用来唤醒等待**某个锁**的所有**挂起的任务**。`等待某个锁`指的是某些需要获取共同的锁的线程，`notifyAll()`可以唤醒这些线程，而不是程序中所有被挂起的线程。

### 死锁

多个并发进程因争夺系统资源而产生相互等待的现象。

四个必要条件：

- 互斥
- 占有且等待
- 不可抢占
- 循环等待

### 免锁容器

免锁容器的策略是：对容器的修改可以与读取操作**同时发生**，只要读取者只能看到完成修改的结果即可。修改时在容器数据结构的某个部分的一个单独的副本上执行的，并且这个副本在修改过程中是不可视的。只有当修改完成时，被修改的结构才会自动地与主数据结构进行交换，之后读取者就可以看到这个修改了。

这些容器允许并发的读取和写入，但是在任何修改完成之前，读取者仍然是不能够看到它们的。

#### 乐观锁

每次拿数据的时候认为别人不会修改，所以不会上锁，但是在更新的时候会判断此期间有没有别人更新这个数据。上述有提到的原子类就是使用了CAS实现的乐观锁。

#### 悲观锁

每次拿数据的时候都认为别人会修改，所以每次拿数据的时候都会上锁。`synchronized`关键字的实现就是悲观锁。

#### CAS(Compare And Swap)技术

CAS是用来实现乐观锁的一种方法，原理见[这里](https://www.jianshu.com/p/ae25eb3cfb5d)。

CAS机制使用3个基本操作数：**内存地址`V`**，**旧的预期值`A`**，**要修改的新值`B`**。

更新一个变量的时候，只有当`A`和`V`的实际值相同时，才会将`V`对应的值修改为`B`。

缺点：

- ABA问题：链表的头在变化了两次后恢复了原值，但是不代表链表就没有发生变化
- 循环时间长开销大
- 只能保证一个共享变量的原子性

未完~