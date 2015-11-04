# Timeline
A simple timeline widget with smooth scrolling and zooming.

The concept is there are two types of items : tasks and events. 
- A task represents some continuous action, and events occur during the task's execution.
- Any event must have a parent task.
- Tasks may have different types, specified by a user.
- There must be an axis for every task type (temporary, going to remove this constraint soon)
- A unique style can be specified for every task type.
- Both tasks and events have time duration. 
- There is a storage for tasks implemented as a separate class

So, the basic example of using the class may look as follows:

```
TaskStoragePtr taskStorage = std::make_shared<TaskStorage>();
SphereTimeLineWidget* timeLineWidget = new SphereTimeLineWidget(taskStorage);

TaskStyle testTaskStyle1(QBrush(Qt::red), QPen(Qt::red), ":/Sphere/Resources/timeline_warning_vector.svg");
TaskStyle testTaskStyle2(QBrush(Qt::blue), QPen(Qt::blue), "");
TaskStyle testTaskStyle3(QBrush(QColor(0, 87, 16)), QPen(QColor(0, 87, 16)), "");

timeLineWidget->addItemType(TEST_ITEM_TYPE_1, testTaskStyle1);
timeLineWidget->addItemType(TEST_ITEM_TYPE_2, testTaskStyle2);
timeLineWidget->addItemType(TEST_ITEM_TYPE_2, testTaskStyle3);

TaskItemPtr taskPtr = std::make_shared<TaskItem>(QDateTime::currentDateTime(), 
                                                 QDateTime::currentDateTime().addMonths(10), 
                                                 0,    //no end time
                                                 true, //task is infinite 
												 "TEST_TASK_NAME",
                                                 TEST_ITEM_TYPE_1 ));
                                             
taskStorage->addTask(taskPtr);

EventItemPtr eventPtr = std::make_shared<EventItem(QDateTime::currentDateTime().addMonth(5), 
                                                   QDateTime::currentDateTime().addMonth(6), 
                                                   OperationSphereItem::EVENT_STATUS_SUCCEDED));

taskStorage->addEvent(eventPtr);
```
