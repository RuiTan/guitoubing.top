---
title: Java - JavaFX学习小记
date: 2018-10-28 00:08:25
tags:
    - JavaFX
categories:
    - Java
---
# JavaFX小记

## 简介

- JavaFX

  `JavaFX`是由[甲骨文(Oracle)公司](https://zh.wikipedia.org/wiki/%E7%94%B2%E9%AA%A8%E6%96%87%E5%85%AC%E5%8F%B8)推出的一系列的产品和技术，主要应用于创建Rich Internet application([RIAs](https://zh.wikipedia.org/wiki/RIA))，它是一个跨平台的桌面应用程序开发框架。

<!-- more -->

- 典型的MVC架构

  - 定义`Model`，使用`javafx.beans`封装类型定义属性类型
  - 使用`fxml`文件创建`View`，利用SceneBuilder工具进行布局
  - 创建`Controller`实现动作操作以及`Model`和`View`的联系

## View

- **创建FXML文件，利用SceneBuilder工具进行布局**

## Model

- **定义`Model`中的`Person`类，使用`Property`和`Bind`**

  `java.beans`包中的对象类型不是标准的Java原语，而是新的封装起来的类，它封装了Java原语并添加了一些额外的功能，`Property`和`Bind`方便我们实现以下功能：当某个属性如`First Name`被改变时，会自动收到通知而修改视图，从而保证视图与数据的同步。当然仅仅声明这种类型是不够的，声明只是为后续操作提供类型前提，还需要进一步操作，可参考[JavaFX文档](https://docs.oracle.com/javase/8/javafx/properties-binding-tutorial/binding.htm)。

  **Person.java**

  ```java
  package com.tanrui.model;

  import java.time.LocalDate;

  import javafx.beans.property.IntegerProperty;
  import javafx.beans.property.ObjectProperty;
  import javafx.beans.property.SimpleIntegerProperty;
  import javafx.beans.property.SimpleObjectProperty;
  import javafx.beans.property.SimpleStringProperty;
  import javafx.beans.property.StringProperty;

  /**
   * Model class for a Person.
   */
  public class Person {

      private final StringProperty firstName;
      private final StringProperty lastName;
      private final StringProperty street;
      private final IntegerProperty postalCode;
      private final StringProperty city;
      private final ObjectProperty<LocalDate> birthday;

      /**
       * Default constructor.
       */
      public Person() {
          this(null, null);
      }

      /**
       * Constructor with some initial data.
       *
       * @param firstName
       * @param lastName
       */
      public Person(String firstName, String lastName) {
          this.firstName = new SimpleStringProperty(firstName);
          this.lastName = new SimpleStringProperty(lastName);

          // Some initial dummy data, just for convenient testing.
          this.street = new SimpleStringProperty("some street");
          this.postalCode = new SimpleIntegerProperty(1234);
          this.city = new SimpleStringProperty("some city");
          this.birthday = new SimpleObjectProperty<LocalDate>(LocalDate.of(1999, 2, 21));
      }

      public String getFirstName() {
          return firstName.get();
      }

      public void setFirstName(String firstName) {
          this.firstName.set(firstName);
      }

      public StringProperty firstNameProperty() {
          return firstName;
      }

      public String getLastName() {
          return lastName.get();
      }

      public void setLastName(String lastName) {
          this.lastName.set(lastName);
      }

      public StringProperty lastNameProperty() {
          return lastName;
      }

      public String getStreet() {
          return street.get();
      }

      public void setStreet(String street) {
          this.street.set(street);
      }

      public StringProperty streetProperty() {
          return street;
      }

      public int getPostalCode() {
          return postalCode.get();
      }

      public void setPostalCode(int postalCode) {
          this.postalCode.set(postalCode);
      }

      public IntegerProperty postalCodeProperty() {
          return postalCode;
      }

      public String getCity() {
          return city.get();
      }

      public void setCity(String city) {
          this.city.set(city);
      }

      public StringProperty cityProperty() {
          return city;
      }

      public LocalDate getBirthday() {
          return birthday.get();
      }

      public void setBirthday(LocalDate birthday) {
          this.birthday.set(birthday);
      }

      public ObjectProperty<LocalDate> birthdayProperty() {
          return birthday;
      }
  }
  ```

- **使用`ObservableList`管理`Person`**

  前一点所述的<u>**后续**</u>操作便是此处了，JavaFX为了实现上述目的即保持视图和数据的同步，引入了一些新的集合类，这里我们用到的是`ObservableList`，`ObservableList`继承了`List`类、实现了`Observable`接口，其实现视图和数据同步的方法是在声明`ObservableList`时为方法传递一个监听器，此监听器需要会通过监听`personData`的变化同步改变视图中对应的值，可参考[ObservableList文档](https://docs.oracle.com/javase/8/javafx/api/javafx/collections/ObservableList.html)

  **Main.java:**

  ```java

  public class Main extends Application {

      /*......Other variables......*/

      /**
       *
       * The data of a observable list of Persons
       */
      private ObservableList<Person> personData = FXCollections.observableArrayList();

      public ObservableList<Person> getPersonData() {
          return personData;
      }

      public Main(){
          personData.add(new Person("Tan", "Rui"));
          personData.add(new Person("Chen", "Chao"));
          personData.add(new Person("Liang", "Chengwei"));
          personData.add(new Person("Xiao", "Xin"));
          personData.add(new Person("Li", "Yang"));
          personData.add(new Person("Chen", "Runqian"));
          personData.add(new Person("Liang", "Yongchao"));
          personData.add(new Person("Luo", "Jihao"));
          personData.add(new Person("Chen", "Zhi"));
          personData.add(new Person("Fan", "Fan"));

      }

      /* ......Other function..... */
  }
  ```

## Controller

### PersonOverviewController.java

```java
package com.tanrui.view;

import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import com.tanrui.Main;
import com.tanrui.model.Person;

public class PersonOverviewController {
    @FXML
    private TableView<Person> personTable;
    @FXML
    private TableColumn<Person, String> firstNameColumn;
    @FXML
    private TableColumn<Person, String> lastNameColumn;

    @FXML
    private Label firstNameLabel;
    @FXML
    private Label lastNameLabel;
    @FXML
    private Label streetLabel;
    @FXML
    private Label postalCodeLabel;
    @FXML
    private Label cityLabel;
    @FXML
    private Label birthdayLabel;

    // Reference to the main application.
    private Main main;

    /**
     * The constructor.
     * The constructor is called before the initialize() method.
     */
    public PersonOverviewController() {
    }

    /**
     * Initializes the controller class. This method is automatically called
     * after the fxml file has been loaded.
     */
    @FXML
    private void initialize() {
        // Initialize the person table with the two columns.
        firstNameColumn.setCellValueFactory(cellData -> cellData.getValue().firstNameProperty());
        lastNameColumn.setCellValueFactory(cellData -> cellData.getValue().lastNameProperty());
    }

    /**
     * Is called by the main application to give a reference back to itself.
     *
     * @param main
     */
    public void setMain(Main main) {
        this.main = main;

        // Add observable list data to the table
        personTable.setItems(main.getPersonData());
    }
}
```

- **`@FXML`注解（Annotation）**

  使用`@FXML`注解可以将操作的属性、方法绑定到`FXML`文件的界面元素，实际上，在属性、方法是非私有的情况下可以不使用`@FXML`注解，但是比起非私有声明，让他们保持私有并用注解标记的方式会更好！

- **`initialize()`方法**

  `initialize()`字面意思可知其是用于初始化对应`FXML`文件中的属性，此方法会在加载`FXML`文件时被自动执行，此时，所有的`FXML`属性都应已被初始化

- **`setCellValueFactory(...)`方法**

  我们对表格列上使用`setCellValueFactory(...)`方法来确定为特定列使用前面`Person`的某个属性。`->`表示使用的是[Lambdas](https://www.oracle.com/webfolder/technetwork/tutorials/obe/java/Lambda-QuickStart/index.html)特性；另外一种方法是使用[PropertyValueFactory](https://docs.oracle.com/javase/8/javafx/api/)(待研究…)。

  这里我们之所以可以使用`cellData -> cellData.getValue().firstNameProperty()`，便是因为之前我们将Person的属性都定义为`javafx.beans`中的封装属性，`firstNameProperty()`等方法都会在声明成`Beans`封装类型时被创建，其遵循了固定的命名规则，这使得我们使用起来特别方便

### 连接Main和PersonOverviewController

- **`showPersonOverview()` 方法**

  **Main.java**

  ```java

      /**
       * Shows the person overview inside the root layout.
       */
      public void showPersonOverview() {
          try {
              // Load person overview.
              FXMLLoader loader = new FXMLLoader();
              loader.setLocation(Main.class.getResource("view/PersonOverview.fxml"));
              AnchorPane personOverview = (AnchorPane) loader.load();

              // Set person overview into the center of root layout.
              rootLayout.setCenter(personOverview);

              // Give the controller access to the main app.
              PersonOverviewController controller = loader.getController();
              controller.setMain(this);

          } catch (IOException e) {
              e.printStackTrace();
          }
      }
  ```

### 将View与Controller绑定

我们还需要为`FXML文件`指定其对应的`Controller`，以及`FXML元素`与`控制器的属性`的对应关系，这是因为FXML文件中的元素只能被对应`Controller`修改更新，若在其他方法中修改会产生运行时错误。例如：在`PersonOverviewController.java`中将某个`Label`返回到`Main.java`中而后在其中修改该`Label`的值，意即在`非FX线程`中执行`FX线程`相关的任务，则会造成当前的线程阻塞，解决方法之一是使用`Platform.runLater()`方法，如下所示，括号中的`FX线程`相关任务便不会阻塞当前进程。

```
Platform.runLater(() -> {
        ………相关FX线程代码………
});
```

当然，最好的选择还是讲`FX线程`任务和其他任务区分开来，将特定的`FXML文件`与对应的`Controller`联系起来，当需要建立联系时可通过之前所说的使用`java.beans`、`ObservableList`等方法实现动态更新视图。

- **为`FXML文件`指定`Controller`**

  在Eclipse中好像有图形化界面直接为`FXML文件`选择`Controller`的操作，但是我使用的是IDEA，没有此功能，只能在源代码中指定，如下所示。

  **PersonOverview.fxml**

  ```xml
  <?xml version="1.0" encoding="UTF-8"?>

  <AnchorPane maxHeight="-Infinity" maxWidth="-Infinity" minHeight="-Infinity" minWidth="-Infinity" prefHeight="300.0" prefWidth="600.0" xmlns="http://javafx.com/javafx/8.0.121" xmlns:fx="http://javafx.com/fxml/1" fx:controller="com.tanrui.view.PersonOverviewController">
      <children>
          <? ... 内容省略 ... ?>
      </children>
  </AnchorPane>
  ```

  如上述代码所述，在顶层节点（此处是`AnchorPane`）标签中添加属性如下：`fx:controller="com.tanrui.view.PersonOverviewController”`，以此为`FXML文件`指定`Controller`

- 为`FXML元素`指定`fx:id`，使其绑定对应的`控制器属性`

  ![image-20181023205748006](/img/image-20181023205748006.png)

  如图，选定特定元素，在右侧界面找到`Code`->`fx:id`，将其对应的控制器属性填入即可

### Details界面更新

- **`showPersonDetails(Person person)`方法**

  `showPersonDetails(Person person)`方法用于使用Person实例的数据填写标签。

  **PersonOverviewController.java**

  ```java
  /**
   * Fills all text fields to show details about the person.
   * If the specified person is null, all text fields are cleared.
   *
   * @param person the person or null
   */
  private void showPersonDetails(Person person) {
      if (person != null) {
          // Fill the labels with info from the person object.
          firstNameLabel.setText(person.getFirstName());
          lastNameLabel.setText(person.getLastName());
          streetLabel.setText(person.getStreet());
          postalCodeLabel.setText(Integer.toString(person.getPostalCode()));
          cityLabel.setText(person.getCity());

          // TODO: We need a way to convert the birthday into a String!
          // birthdayLabel.setText(...);
      } else {
          // Person is null, remove all the text.
          firstNameLabel.setText("");
          lastNameLabel.setText("");
          streetLabel.setText("");
          postalCodeLabel.setText("");
          cityLabel.setText("");
          birthdayLabel.setText("");
      }
  }
  ```

- **监听用户在人员表中的选择**

  **PersonOverviewController.java**

  ```java
  @FXML
  private void initialize() {
      // Initialize the person table with the two columns.
      firstNameColumn.setCellValueFactory(
              cellData -> cellData.getValue().firstNameProperty());
      lastNameColumn.setCellValueFactory(
              cellData -> cellData.getValue().lastNameProperty());

      // Clear person details.
      showPersonDetails(null);

      // Listen for selection changes and show the person details when changed.
      personTable.getSelectionModel().selectedItemProperty().addListener(
              (observable, oldValue, newValue) -> showPersonDetails(newValue));
  }
  ```

### 删除按钮事件

我们的界面已经包含了一个删除的按钮 ，但是并没有为其制定实际的响应操作，因此我们定义一个响应函数，如下：

**PersonOverviewController.java**:

```java
/**
     * Called when the user clicks on the delete button.
     */
    @FXML
    private void handleDeletePerson() {
        int selectedIndex = personTable.getSelectionModel().getSelectedIndex();
        if (selectedIndex >= 0){
            personTable.getItems().remove(selectedIndex);
        }
        else{
            new ShowDialog(main.getPrimaryStage(), Alert.AlertType.WARNING, "No Person Selected", "Please select a person in the table.").ShowSpecificDialog();
        }
    }
```

#### 错误处理

从上述代码可以看到我们使用了条件判断语句来判断`selectedIndex`的值，当其小于0时，正常情况我们应该会让其抛出`ArrayIndexOutOfBoundsException`异常，但是我们想尽量简洁明了的将错误或者警告信息展示给用户，因此这里我们使用了`controlsfx`包，用于弹出各类提示框（可在[ControlsFX](http://fxexperience.com/controlsfx/)官网获取）。

`controlsfx`有两个主要的版本，同时对于不同的版本，二者的用法也不同：

- 对于Java 8，需要下载[ControlsFX 8.40.14](http://fxexperience.com/downloads/controlsfx-8-40-14/)包
- 对于Java 9及以上，需要下载[ControlsFX 9.0.0](http://fxexperience.com/downloads/controlsfx-9-0-0/)包

我们这里用到的是Java 10，因此使用`ControlsFX 9.0.0`，使用方法如下：

**ShowDialog.java**:

```java
package com.tanrui.util;

import javafx.scene.control.Alert;
import javafx.stage.Stage;

/**
 * Util to create and show Dialog.
 *
 * @author Tan Rui
 */
public class ShowDialog {

    private Stage stage;
    private Alert.AlertType type;
    private String title;
    private String message;

    public ShowDialog(Stage stage, Alert.AlertType type, String title, String message){
        this.stage = stage;
        this.type = type;
        this.title = title;
        this.message = message;
    }

    public void ShowSpecificDialog(){
        Alert dlg = new Alert(type);
        dlg.initOwner(stage);
        dlg.setTitle(title);
        dlg.getDialogPane().setContentText(message);
        dlg.show();
    }
}
```

**PersonOverviewController.java**

```java
/**
     * Called when the user clicks on the delete button.
     */
    @FXML
    private void handleDeletePerson() {
        int selectedIndex = personTable.getSelectionModel().getSelectedIndex();
        if (selectedIndex >= 0){
            personTable.getItems().remove(selectedIndex);
        }
        else{
            new ShowDialog(main.getPrimaryStage(), Alert.AlertType.WARNING, "No Person Selected", "Please select a person in the table.").ShowSpecificDialog();
        }
    }
```

### 新建和编辑对话框

> Tips：创建一个新的界面、新的Stage（承载新的View时），步骤一般都是：
>
> 1. 创建FXML文件，使用SceneBuilder编辑界面；
> 2. 创建对应的Controller，对FXML中的元素指定对应的属性。主要是为展示型元素指定数据、为控制型元素指定动作等；
> 3. 连接FXML文件和Controller文件、连接FXML中的元素和Controller中的属性；
> 4. 在Main函数中加载该控制器

为之前的`New`和`Edit`按钮添加动作，弹出对话框（新的Stage）。

#### 设计对话框

创建`PersonEditDialog.fxml`，完成弹出对话框的设计：

![image-20181027150559447](/img/image-20181027150559447.png)

#### 创建控制器

为对话框创建控制器`PersonEditDialogController.java`。

**PersonEditDialogController.java：**

```java
package com.tanrui.view;

import com.tanrui.util.ShowDialog;
import javafx.fxml.FXML;
import javafx.scene.control.Alert;
import javafx.scene.control.TextField;
import javafx.stage.Stage;

import com.tanrui.model.Person;
import com.tanrui.util.DateUtil;


/**
 * Dialog to edit details of a person.
 *
 * @author Marco Jakob
 */
public class PersonEditDialogController {

    @FXML
    private TextField firstNameField;
    @FXML
    private TextField lastNameField;
    @FXML
    private TextField streetField;
    @FXML
    private TextField postalCodeField;
    @FXML
    private TextField cityField;
    @FXML
    private TextField birthdayField;


    private Stage dialogStage;
    private Person person;
    private boolean okClicked = false;

    /**
     * Initializes the controller class. This method is automatically called
     * after the fxml file has been loaded.
     */
    @FXML
    private void initialize() {
    }

    /**
     * Sets the stage of this dialog.
     *
     * @param dialogStage
     */
    public void setDialogStage(Stage dialogStage) {
        this.dialogStage = dialogStage;
    }

    /**
     * Sets the person to be edited in the dialog.
     *
     * @param person
     */
    public void setPerson(Person person) {
        this.person = person;

        firstNameField.setText(person.getFirstName());
        lastNameField.setText(person.getLastName());
        streetField.setText(person.getStreet());
        postalCodeField.setText(Integer.toString(person.getPostalCode()));
        cityField.setText(person.getCity());
        birthdayField.setText(DateUtil.format(person.getBirthday()));
        birthdayField.setPromptText("dd.mm.yyyy");
    }

    /**
     * Returns true if the user clicked OK, false otherwise.
     *
     * @return
     */
    public boolean isOkClicked() {
        return okClicked;
    }

    /**
     * Called when the user clicks ok.
     */
    @FXML
    private void handleOk() {
        if (isInputValid()) {
            person.setFirstName(firstNameField.getText());
            person.setLastName(lastNameField.getText());
            person.setStreet(streetField.getText());
            person.setPostalCode(Integer.parseInt(postalCodeField.getText()));
            person.setCity(cityField.getText());
            person.setBirthday(DateUtil.parse(birthdayField.getText()));

            okClicked = true;
            dialogStage.close();
        }
    }

    /**
     * Called when the user clicks cancel.
     */
    @FXML
    private void handleCancel() {
        dialogStage.close();
    }

    /**
     * Validates the user input in the text fields.
     *
     * @return true if the input is valid
     */
    private boolean isInputValid() {
        String errorMessage = "";

        if (firstNameField.getText() == null || firstNameField.getText().length() == 0) {
            errorMessage += "No valid first name!\n";
        }
        if (lastNameField.getText() == null || lastNameField.getText().length() == 0) {
            errorMessage += "No valid last name!\n";
        }
        if (streetField.getText() == null || streetField.getText().length() == 0) {
            errorMessage += "No valid street!\n";
        }

        if (postalCodeField.getText() == null || postalCodeField.getText().length() == 0) {
            errorMessage += "No valid postal code!\n";
        } else {
            try {
                Integer.parseInt(postalCodeField.getText());
            } catch (NumberFormatException e) {
                errorMessage += "No valid postal code (must be an integer)!\n";
            }
        }

        if (cityField.getText() == null || cityField.getText().length() == 0) {
            errorMessage += "No valid city!\n";
        }

        if (birthdayField.getText() == null || birthdayField.getText().length() == 0) {
            errorMessage += "No valid birthday!\n";
        } else {
            if (!DateUtil.validDate(birthdayField.getText())) {
                errorMessage += "No valid birthday. Use the format dd.mm.yyyy!\n";
            }
        }

        if (errorMessage.length() == 0) {
            return true;
        } else {
            new ShowDialog(dialogStage, Alert.AlertType.ERROR, "Invalid Fields", "Please correct invalid fields").ShowSpecificDialog();
            return false;
        }
    }
}
```

关于该控制器的一些事情应该注意：

1. `setPerson(…)`方法可以从其它类中调用，用来设置编辑的人员。
2. 当用户点击OK按钮时，调用`handleOK()`方法。首先，通过调用`isInputValid()`方法做一些验证。只有验证成功，Person对象使用输入的数据填充。这些修改将直接应用到Person对象上，传递给`setPerson(…)`。
3. 布尔值`okClicked`被使用，以便调用者决定用户是否点击OK或者Cancel按钮。

#### 连接视图和控制器

使用已经创建的视图（FXML）和控制器，需要连接到一起。

1. 使用SceneBuilder打开`PersonEditDialog.fxml`文件
2. 在左边的*Controller*组中选择`PersonEditDialogController`作为控制器类
3. 设置所有**TextField**的`fx:id`到相应的控制器字段上。
4. 设置两个按钮的**onAction**到相应的处理方法上。

#### 在Main中部署该控制器

**Main.java:**

```java
/**
 * Opens a dialog to edit details for the specified person. If the user
 * clicks OK, the changes are saved into the provided person object and true
 * is returned.
 *
 * @param person the person object to be edited
 * @return true if the user clicked OK, false otherwise.
 */
public boolean showPersonEditDialog(Person person) {
    try {
        // Load the fxml file and create a new stage for the popup dialog.
        FXMLLoader loader = new FXMLLoader();
        loader.setLocation(Main.class.getResource("view/PersonEditDialog.fxml"));
        AnchorPane page = (AnchorPane) loader.load();

        // Create the dialog Stage.
        Stage dialogStage = new Stage();
        dialogStage.setTitle("Edit Person");
        dialogStage.initModality(Modality.WINDOW_MODAL);
        dialogStage.initOwner(primaryStage);
        Scene scene = new Scene(page);
        dialogStage.setScene(scene);

        // Set the person into the controller.
        PersonEditDialogController controller = loader.getController();
        controller.setDialogStage(dialogStage);
        controller.setPerson(person);

        // Show the dialog and wait until the user closes it
        dialogStage.showAndWait();

        return controller.isOkClicked();
    } catch (IOException e) {
        e.printStackTrace();
        return false;
    }
}
```

为主界面中`New`和`Edit`按钮创建OnAction方法，这些方法将从`Main`中调用`showPersonEditDialog(…)`方法。

**PersonOverviewController.java:**

```java
    /**
     * Called when the user clicks the new button. Opens a dialog to edit
     * details for a new person.
     */
    @FXML
    private void handleNewPerson() {
        Person tempPerson = new Person();
        boolean okClicked = main.showPersonEditDialog(tempPerson);
        if (okClicked) {
            main.getPersonData().add(tempPerson);
        }
    }

    /**
     * Called when the user clicks the edit button. Opens a dialog to edit
     * details for the selected person.
     */
    @FXML
    private void handleEditPerson() {
        Person selectedPerson = personTable.getSelectionModel().getSelectedItem();
        if (selectedPerson != null) {
            boolean okClicked = main.showPersonEditDialog(selectedPerson);
            if (okClicked) {
                showPersonDetails(selectedPerson);
            }

        } else {
            new ShowDialog(main.getPrimaryStage(), Alert.AlertType.WARNING, "No Person Selected", "Please select a person in the table.").ShowSpecificDialog();
        }
    }
```

而后在`PersonOverview.fxml`中为New和Edit两个按钮绑定对应的OnAction方法：

![image-20181027164439676](/img/image-20181027164439676.png)

### 数据持久化

我们有很多种方法来实现应用数据的持久化，例如：

- 使用数据库存储
- 使用Json文件存储
- 使用XML文件存储
- ……

这里我们使用XML文件格式存储应用数据。之前的我们应用的数据都只是存在内存中，内存的特性使得关闭应用程序后数据便会丢失，因此我们下面要做的就是：

1. 每次打开应用可加载上一次的用户数据
2. 用户可选择保存当前数据到指定XML文件
3. 用户可选择从指定XML文件加载数据

#### 使用Preferences保存应用状态

`Java`提供了`Preferences`类来帮助我们存储用户配置（本例中是XML数据文件的路径，用于下次打开从该文件中加载），`Preferences`类底层对各类操作系统进行了封装（实际上是`Windows系统`、`OS X系统`和`类Unix文件系统`三种），用户配置在`Windows系统`上可能保存在注册表中、在`类Unix文件系统`上可能保存在`/tmp`下的某个隐藏文件中，而对于使用者来说这些实现细节都不必考虑，只需知道`Preferences`类是用来保存用户配置即可。用法如下：

**Main.java:**

```java
/**
     * Returns the person file preference, i.e. the file that was last opened.
     * The preference is read from the OS specific registry. If no such
     * preference can be found, null is returned.
     *
     * @return
     */
    public File getPersonFilePath() {
        Preferences prefs = Preferences.userNodeForPackage(Main.class);
        String filePath = prefs.get("filePath", null);
        if (filePath != null) {
            return new File(filePath);
        } else {
            return null;
        }
    }

    /**
     * Sets the file path of the currently loaded file. The path is persisted in
     * the OS specific registry.
     *
     * @param file the file or null to remove the path
     */
    public void setPersonFilePath(File file) {
        Preferences prefs = Preferences.userNodeForPackage(Main.class);
        if (file != null) {
            prefs.put("filePath", file.getPath());
            // Update the stage title.
            primaryStage.setTitle("AddressApp - " + file.getName());
        } else {
            prefs.remove("filePath");
            // Update the stage title.
            primaryStage.setTitle("AddressApp");
        }
    }
```

#### 使用JAXB

`JAXB包`是Java中提供的对数据进行`编列(marshall)`成XML文件以及对XML文件`反编列(unmarshall)`为数据结构的包，`Java SE`中有如下支持类型：`JAXB 2.0`是`JDK 1.6`的组成部分。`JAXB 2.2.3`是`JDK 1.7以上`的组成部分，而实际上在`Java 9`之后就已将`JAXB`包移除，因此使用时需添加额外的lib包，详情可见博客[真正解决方案：java.lang.ClassNotFoundException: javax.xml.bind.JAXBException](https://blog.csdn.net/hadues/article/details/79188793)。

##### JAXB模型类

我们希望持久化的数据应该是`Main`中的`personData`，而`JAXB`有以下要求：

- 使用`@XmlRootElement`定义`XML根元素`的名称
- 使用`@XmlElement`指定一个`XML元素`，可选

而`Main`中的`personData`是`ObservableList`类型，由于`ObservableList`类型不支持添加注解，因此我们需要创建另外一个能保存`Person`列表同时又能存储为`XML文件`的类，如下。

**PersonListWrapper.java:**

```java
package com.tanrui.model;

import java.util.List;

import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * Helper class to wrap a list of persons. This is used for saving the
 * list of persons to XML.
 */
@XmlRootElement(name = "persons")
public class PersonListWrapper {

    private List<Person> persons;

    @XmlElement(name = "person")
    public List<Person> getPersons() {
        return persons;
    }

    public void setPersons(List<Person> persons) {
        this.persons = persons;
    }
}
```

##### 使用JAXB读写数据到XML文件

我们将读写XML文件的逻辑放到`Main类`中，`Controller`在用到相应的逻辑时，直接调用`Main`中的方法即可。

**Main.java:**

```java

    /**
     * Loads person data from the specified file. The current person data will
     * be replaced.
     *
     * @param file
     */
    public void loadPersonDataFromFile(File file) {
        try {
            JAXBContext context = JAXBContext
                    .newInstance(PersonListWrapper.class);
            Unmarshaller um = context.createUnmarshaller();

            // Reading XML from the file and unmarshalling.
            PersonListWrapper wrapper = (PersonListWrapper) um.unmarshal(file);

            personData.clear();
            personData.addAll(wrapper.getPersons());

            // Save the file path to the registry.
            setPersonFilePath(file);

        } catch (Exception e) { // catches ANY exception
            new ShowDialog(this.getPrimaryStage(), Alert.AlertType.ERROR, "Error", "Could not save data to file:\n" + file.getPath()).ShowSpecificDialog();
        }
    }

    /**
     * Saves the current person data to the specified file.
     *
     * @param file
     */
    public void savePersonDataToFile(File file) {
        try {
            JAXBContext context = JAXBContext.newInstance(PersonListWrapper.class);
            Marshaller m = context.createMarshaller();
            m.setProperty(Marshaller.JAXB_FORMATTED_OUTPUT, true);

            // Wrapping our person data.
            PersonListWrapper wrapper = new PersonListWrapper();
            wrapper.setPersons(personData);

            // Marshalling and saving XML to the file.
            m.marshal(wrapper, file);

            // Save the file path to the registry.
            setPersonFilePath(file);
        } catch (Exception e) { // catches ANY exception
            new ShowDialog(this.getPrimaryStage(), Alert.AlertType.ERROR, "Error", "Could not save data to file:\n" + file.getPath()).ShowSpecificDialog();
        }
    }
```

`编组(marshall):savePersonDataToFile(…)`和`解组(unmarshall):loadPersonDataFromFile(…)`已准备好，下面在界面中使用它。

#### 创建打开和保存菜单

##### 为File菜单添加子项

![image-20181027232418408](/img/image-20181027232418408.png)

##### 处理菜单相应动作

`Controller`中使用`FileChooser`的方法，`FileChooser`同样封装了不同操作系统的具体实现，使用者仅需调用接口即可。

本类中使用了`FileChooser.ExtensionFilter`，对文件系统中文件进行过滤，保留`.xml`结尾的文件。

当用户选择特定文件而后点击`打开`按钮时，会返回该文件，否则返回`Null`。

```java
package com.tanrui.view;

import com.tanrui.Main;
import com.tanrui.util.ShowDialog;
import javafx.fxml.FXML;
import javafx.scene.control.Alert;
import javafx.stage.FileChooser;

import java.io.File;

/**
 * The controller for the root layout. The root layout provides the basic
 * application layout containing a menu bar and space where other JavaFX
 * elements can be placed.
 */
public class RootLayoutController {

    // Reference to the main application
    private Main main;

    /**
     * Is called by the main application to give a reference back to itself.
     *
     * @param main
     */
    public void setMain(Main main) {
        this.main = main;
    }

    /**
     * Creates an empty address book.
     */
    @FXML
    private void handleNew() {
        main.getPersonData().clear();
        main.setPersonFilePath(null);
    }

    /**
     * Opens a FileChooser to let the user select an address book to load.
     */
    @FXML
    private void handleOpen() {
        FileChooser fileChooser = new FileChooser();

        // Set extension filter
        FileChooser.ExtensionFilter extFilter = new FileChooser.ExtensionFilter(
                "XML files (*.xml)", "*.xml");
        fileChooser.getExtensionFilters().add(extFilter);

        // Show save file dialog
        File file = fileChooser.showOpenDialog(main.getPrimaryStage());

        if (file != null) {
            main.loadPersonDataFromFile(file);
        }
    }

    /**
     * Saves the file to the person file that is currently open. If there is no
     * open file, the "save as" dialog is shown.
     */
    @FXML
    private void handleSave() {
        File personFile = main.getPersonFilePath();
        if (personFile != null) {
            main.savePersonDataToFile(personFile);
        } else {
            handleSaveAs();
        }
    }

    /**
     * Opens a FileChooser to let the user select a file to save to.
     */
    @FXML
    private void handleSaveAs() {
        FileChooser fileChooser = new FileChooser();

        // Set extension filter
        FileChooser.ExtensionFilter extFilter = new FileChooser.ExtensionFilter(
                "XML files (*.xml)", "*.xml");
        fileChooser.getExtensionFilters().add(extFilter);

        // Show save file dialog
        File file = fileChooser.showSaveDialog(main.getPrimaryStage());

        if (file != null) {
            // Make sure it has the correct extension
            if (!file.getPath().endsWith(".xml")) {
                file = new File(file.getPath() + ".xml");
            }
            main.savePersonDataToFile(file);
        }
    }

    /**
     * Opens an about dialog.
     */
    @FXML
    private void handleAbout() {
        new ShowDialog(main.getPrimaryStage(), Alert.AlertType.INFORMATION, "About", "Author: Tan\\nWebsite: https://guitoubing.top").ShowSpecificDialog();
    }

    /**
     * Closes the application.
     */
    @FXML
    private void handleExit() {
        System.exit(0);
    }

    /**
     * Opens the birthday statistics.
     */
    @FXML
    private void handleShowBirthdayStatistics() {
        main.showBirthdayStatistics();
    }
}
```

##### 连接FXML文件和Controller、绑定菜单和对应动作

![image-20181027233726178](/img/image-20181027233726178.png)

![image-20181027233529314](/img/image-20181027233529314.png)

##### 在Main中部署该控制器

**Main.java:**

```java
 /**
     * Initializes the root layout.
     */
    public void initRootLayout() {
        try {
            // Load root layout from fxml file.
            FXMLLoader loader = new FXMLLoader();
            loader.setLocation(Main.class.getResource("view/RootLayout.fxml"));
            rootLayout = (BorderPane) loader.load();

            // Show the scene containing the root layout.
            Scene scene = new Scene(rootLayout);
            primaryStage.setScene(scene);

            RootLayoutController controller = loader.getController();
            controller.setMain(this);

            primaryStage.show();
        } catch (IOException e) {
            e.printStackTrace();
        }

        File file = getPersonFilePath();
        if (file != null){
            loadPersonDataFromFile(file);
        }
    }
```

## 参考资料

1. [code.makery —— JavaFX中文教程](https://code.makery.ch/)
2. [JavaFX Tutorial](https://www.tutorialspoint.com/javafx/)
3. [真正解决方案：java.lang.ClassNotFoundException: javax.xml.bind.JAXBException](https://blog.csdn.net/hadues/article/details/79188793)
4. [fxexperience —— ControlFX](http://fxexperience.com/controlsfx/)
5. [Java SE8 —— Lambda](https://www.oracle.com/webfolder/technetwork/tutorials/obe/java/Lambda-QuickStart/index.html)
6. …………

## 写在后面

本博主要是在学习[code.makery —— JavaFX中文教程](https://code.makery.ch/)博客中对于JavaFX的教程，跟着博主的项目逻辑和代码自己过了一遍，对一些由于版本不兼容（博主使用的是`JDK 8u40`，我这里使用的是`Java 10 2018-03-20`）造成的问题进行了解决，同时对项目过程中一些功能进行了拓展学习，研究了很多用到的包源码，收获颇多。可点击[JavaFX-Test](http://getme.guitoubing.top/JavaFX_PRE.zip)中获取源码。

希望藉此次`JavaFX`学习开启我的Java源码学习之旅，道阻且长！
