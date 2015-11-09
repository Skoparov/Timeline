#ifndef TIMELINE_H
#define TIMELINE_H

#define DEBUG

#include <QMap>
#include <QHash>
#include <QRect>
#include <QPair>
#include <QPoint>
#include <QMutex>
#include <QDebug>
#include <QTimer>
#include <QLabel>
#include <QWidget>
#include <QAction>
#include <QPointF>
#include <QString>
#include <QPainter>
#include <QTimeLine>
#include <QDateTime>
#include <QTabWidget>
#include <QWheelEvent>
#include <QPushButton>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>

#include <memory>

inline uint qHash(const QRect& rect, uint seed = 0)
{
    return qHash(rect.x()) ^ qHash(rect.width()) ^ qHash(seed);
}

inline uint qHash(const QPair<QDateTime, QDateTime>& data, uint seed = 0)
{
    return qHash(data.first.toMSecsSinceEpoch()) ^ qHash(data.second.toMSecsSinceEpoch()) ^ qHash(seed);
}


class AbstractItem;
class TaskItem;
class EventItem;
class TaskStorage;
struct TaskStyle;

typedef std::shared_ptr<AbstractItem> TimeLineItemPtr;
typedef std::shared_ptr<TaskItem> TaskItemPtr;
typedef std::shared_ptr<EventItem> EventItemPtr;
typedef std::shared_ptr<TaskStorage> TaskStoragePtr;
typedef std::shared_ptr<TaskStyle> TaskStylePtr;

enum TimeLineTaskType
{
    TASK_TYPE_TEST_EXAMPLE,
    TL_TASK_TYPE_INVALID
};

//////////////////////////////////////////////////////////////////////////////
///////////////             AbstractTimeLineItem         /////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* Base class for timeline objects. Must be aware of all it's inheritors
*/

class AbstractItem
{
public:
    enum ItemType
    {
        ITEM_TYPE_EVENT,
        ITEM_TYPE_TASK,
        ITEM_TYPE_INVALID,
    };

protected:
    QDateTime mStartTime;
    QDateTime mEndTime;

public:
    AbstractItem(QDateTime startTime = QDateTime(), QDateTime endTime = QDateTime());
    virtual ~AbstractItem() {};

    void setStartTime(const QDateTime startTime);
    void setEndTime(const QDateTime endTime);

    //getters
    QDateTime getStartTime() const;
    QDateTime getEndTime() const;
    QPair<QDateTime, QDateTime> getIntersection(const QDateTime& startTime, const QDateTime& endTime) const;  //Returns intersection with the object's time interval
    virtual ItemType getItemType() const = 0;
};

//////////////////////////////////////////////////////////////////////////////
///////////////             EventItem                    /////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* Event is a task's subitem representing some specific action done by the task
* Arbitrary, tasks can have no events at all
*/

class EventItem : public AbstractItem
{
public:
    enum EventStatus
    {
        EVENT_STATUS_SUCCEDED,
        EVENT_STATUS_ABORTED,
        EVENT_STATUS_FAILURE,
        EVENT_STATUS_INVALID
    };

private:
    EventStatus mStatus; // Result of the task
    TaskItemPtr mParentTask;

public:
    EventItem(QDateTime startTime = QDateTime(), QDateTime endTime = QDateTime(), EventStatus stat = EVENT_STATUS_INVALID);

    //setters
    bool setParentTask(TaskItemPtr task);

    //getters
    const TaskItemPtr getParentTask() const;
    EventStatus getStatus() const;
    ItemType getItemType() const;
};


//////////////////////////////////////////////////////////////////////////////
///////////////             TaskItem                     /////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* A task is the concept representing some general action on the timeline.
* It may my comprised of discrete events or represent a single continuous action
* Every task type has it's own axis to paint on, style etc
*/

class TaskItem : public AbstractItem
{
private:
    quint64 mTaskId;
    bool mIsInfinite;
    QString mTaskName;
    TimeLineTaskType mTaskType;
    QMap<QDateTime, EventItemPtr> mEvent;
    QMap<QDateTime, EventItemPtr> mEventsWithInfoSigh;      // Events with icons

public:
    TaskItem(const QDateTime startTime = QDateTime(),
             const QDateTime endTime = QDateTime(),
             const quint64& taskId = -1,
             const bool& isInfinite = false,
             const QString taskName = QString(),
             const TimeLineTaskType& taskType = TL_TASK_TYPE_INVALID);

    //setters
    bool addEvent(EventItemPtr event);

    //getters
    quint64 getTaskId() const;
    ItemType getItemType() const;
    TimeLineTaskType getTaskType() const;
    QString getTaskName() const;

    bool isInfinite() const;

    quint32 eventCount() const;
    const QMap<QDateTime, EventItemPtr>& getEvents() const;
    const QMap<QDateTime, EventItemPtr>& getEventsWithInfoIcon() const;
};

//////////////////////////////////////////////////////////////////////////////
///////////////             TimeLineGrid                //////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* Class responsible for painting timeline's interface
*/

class TimeLineGrid : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

private:
    static const int second = 1000;
    static const int minute = second * 60;
    static const int hour = minute * 60;
    static const int day = hour * 24;
    static const int week = day * 7;

    static const int mMaxNumberOfUnits = 30;

    static const QString mTimeFormat;
    static const QString mDayFormat;
    static const double mOverlayOpacity;

public:
    struct TimeLineGridStyle
    {
        QColor currMarkColor;                       // Color of the current time mark and it's text. Default - Qt::red
        QColor mouseMarkColor;                      // Color of the mouse mark aand it's text. Default - Qt::blue
        QColor timeMarksTextColor;                  // Text marks color. Default - 115,115,115 (light grey)
        QColor borderColor;                         // Frame color. Default - Qt::black

        TimeLineGridStyle(const QColor& currentTimeMarkColor = Qt::red,
                          const QColor& mouseTimeMarkColor = Qt::blue,
                          const QColor& timeMarksColor = QColor(110, 110, 110),
                          const QColor& gridBorderColor = QColor(0, 0, 0, 150)) :
                          currMarkColor(currentTimeMarkColor), mouseMarkColor(mouseTimeMarkColor),
                          timeMarksTextColor(timeMarksColor), borderColor(gridBorderColor){}
    };

    struct TimeLineGridSettings
    {
        quint32 borderIndentY;                      // Item painting region's vertical indent (from the borders of the widget, px)
        quint32 borderIndentX;			            // Item painting region's horizontal indent (from the borders of the widget, px)
        quint64 maximumScale;                       // Max zoom time interval - MINUTE
        quint64 minimumScale;                       // Min zoom time interval - WEEK

        TimeLineGridSettings(const quint32 borderIndentHorisontal = 0,
                             const quint32 borderIndentVertical = 15,
                             const quint64 maximumTimeScale = minute,
                             const quint64 minimumTimeScale = week) :
                             borderIndentX(borderIndentHorisontal),
                             borderIndentY(borderIndentVertical),
                             maximumScale(maximumTimeScale),
                             minimumScale(minimumTimeScale) {}
    };

private:
    QDateTime mTimeCenterMark;
    quint64 mTimeDelta;                               // Current scale - msec from the central mark to both borders
    QPoint mMousePos;
    QSizeF mSize;                                     // Current grid scale

    TimeLineGridStyle mStyle;
    TimeLineGridSettings mSettings;

private:
   void drawMarks(QPainter* painter);
   void drawCurrTimeMark(const double& msecPerPixel, const quint64& currTime, const quint64& endTime,
                         const quint64& startTime, QPair<int, int>& currTimeMarkBorders,
                         const QString textFormat, const QFontMetrics& fm, QPainter* painter);

   void drawMouseTimeMark(QPainter* painter);
   void drawGridMarks(const QFontMetrics& fm, const double& msecPerPixel,
                      quint64& startTime, const QString textFormat,
                      QPair<int, int> &currTimeMarkBorders, QPainter *painter);

public:
    TimeLineGrid(QGraphicsItem* parent = 0);

    //setters
    void setStyle(const TimeLineGridStyle& style);
    void setSettings(const TimeLineGridSettings& settings);

    void setSize(const QSizeF& size, const QPointF& pos);
    bool setTimeRange(const QDateTime& centralTime, const quint64& timeDelta);
    void setMousePos(const QPoint& pos, bool isDragging = false);

    //getters
    QDateTime getTimeMark() const;
    quint64 getTimeDelta() const;
    QPoint getMousePos() const;
    TimeLineGridSettings getSettings() const;
    TimeLineGridStyle getStyle() const;

    quint64 calculateStep(const int& maxNumberOfTextMarks);
    void paintText(bool topBottom, int xPos, QString text, QPainter* painter, QColor color);
    QRectF boundingRect() const;
    QRect graphicsRect() const;                        /**< Timeline item painting region rect */

protected:
    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);

signals:
    void rangeChanged(QDateTime startTime, QDateTime endTime);
};


//////////////////////////////////////////////////////////////////////////////
///////////////	                 TaskStorage            //////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* Stores tasks and their events
*/

class TaskStorage
{
public:
    TaskStorage(){};

    bool addTask(const TaskItemPtr task);
    void removeTask(const quint64& taskId);
    bool addEvent(const quint32 taskId, const EventItemPtr event);
    void clear();

    TaskItemPtr getTask(const quint64& taskId);
    EventItemPtr getEvent(const quint64& taskId, const QDateTime& startTime);
    const QHash<quint64, TaskItemPtr> getTasks();

    void lock();
    void unlock();

private:
    QHash<quint64, TaskItemPtr> mTasks;                       // All added tasks
    QMutex mMutex;
};

//////////////////////////////////////////////////////////////////////////////
///////////////             TimeLineItems               //////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* Paints timeline items
*/

class TimeLineWidget;

class TimeLineItems : public QGraphicsItem
{
private:
    struct VisibleItem
    {
        TimeLineItemPtr item;
        TaskStylePtr style;
        QRect rect;

        VisibleItem(TimeLineItemPtr timeLineItem, const TaskStylePtr itemStyle, const QRect& itemRect) :
                   item(timeLineItem),
                   style(itemStyle),
                   rect(itemRect){}
    };

public:
    struct TimeLineItemsStyle
    {
        QColor backgroundColor;
        QColor selectedItemColor;                                 // Brush to paint currently selected item
        QColor borderColor;                                       // Pen to draw item's border
        qreal axisOpacity;                                        // Item axis opacity
        qreal taskPaintOpacity;
        qreal eventPaintOpacity;

        TimeLineItemsStyle(
            const QColor& backgroundFillColor = QColor(209, 209, 209, 100),
            const QColor& selectedItemFillColor = QColor(255, 255, 110), //soft yellow
            const QColor& borderPenColor = QColor(0, 0, 0, 150),
            const qreal& axisLineOpacity = 0.3,
            const qreal& taskItemOpacity = 0.5,
            const qreal& eventItemOpacity = 1) :
            backgroundColor(backgroundFillColor),
            selectedItemColor(selectedItemFillColor),
            borderColor(borderPenColor),
            axisOpacity(axisLineOpacity),
            taskPaintOpacity(taskItemOpacity),
            eventPaintOpacity(eventItemOpacity) {}
    };

    struct TimeLineItemsSettings
    {
        quint64 eventsVisibleScale;					          // Minimum scale at which events are still visible. Default - 20 мин
        double infoHeightPortion;                             // Icon area height / total item painting area height. Default - 0.25
        double taskHeightPortion;                             // Task item height / Distance between axis.  Default - 0.25
        double eventsHeightPortion;                           // Event item height / Distance between axis.  Default - 0.5

        TimeLineItemsSettings(const quint64& eventsShowedScale = 1000 * 60 * 10 * 2, //20 min
                             const double& infoAreaHeightPortion = 0.25,
                             const double& taskHeightToAxisDeltaPortion = 0.25,
                             const double& eventHeightToAxisDeltaPortion = 0.75) :
                             eventsVisibleScale(eventsShowedScale),
                             infoHeightPortion(infoAreaHeightPortion),
                             taskHeightPortion(taskHeightToAxisDeltaPortion),
                             eventsHeightPortion(eventHeightToAxisDeltaPortion) {}
    };

private:
    TaskStoragePtr mTaskStorage;
    QList<VisibleItem> mVisibleItems;                         // Currently visible objects
    QMap<int, TaskStylePtr> mInfoMarks;			              // Info icons and their styles
    QHash<TimeLineTaskType, TaskStylePtr> mItemStyles;        // Task styles */
    TimeLineItemPtr mSelectedItem;                            // Currently selected object
    QDateTime mCentralTime;
    quint64 mTimeDelta;                                       // Current scale - number of msec form the center to any border*/
    QSizeF mSize;                                             // Current area size */

    TimeLineItemsStyle mStyle;
    TimeLineItemsSettings mSettings;

private:
    void calculateVisibleItems();
    void paintVisibleItems(QPainter* painter);
    void drawAxis(const quint16& resultAreaHeight, QPainter* painter);
    void paintIcons(const quint16& resultAreaHeight, QPainter* painter);

public:
    TimeLineItems(TaskStoragePtr tasks, QGraphicsItem * parent = 0);

    //setters
    void addItemType(const TimeLineTaskType type, const TaskStyle& style);
    void setTime(const QDateTime& centralTime, const quint64& timeDelta);
    void setSize(const QSizeF& size, const QPointF& pos);
    void setSelectedItem(const TimeLineItemPtr item);
    void setSettings(const TimeLineItemsSettings& settings);
    void setStyle(const TimeLineItemsStyle& style);

    //getters
    QList<TimeLineItemPtr> getItemUnderPos(QPoint& pos);     // Retrieve the list of objects under the pos
    TimeLineItemsSettings getSettings() const;
    TimeLineItemsStyle getStyle() const;

    //graphic  
    void paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
    QRectF boundingRect() const;
};

//////////////////////////////////////////////////////////////////////////////
///////////////         SphereTimeLineScaler            //////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* Класс, отвечающий за вычисление параметров масштабирования таймлайна
*/

class SphereTimeLineScaler : public QObject
{
    Q_OBJECT

    QTimeLine* mZoomingTimeLine;                       // Updates the scale when zooming
    int mScheduledScaling;                             // Planned elementary zooming actions
    static const int mDefaultScale = 600000;           // Default scale - 10 MINUTES

    double mZoomStepRelaxationCoeff;                   // Elementary scaling coefficient
    quint16 mZoomStepTime;                             // Scaling time
    quint16 mElementalZoomTime;                        // Elementary scaling time

public:
    SphereTimeLineScaler(QObject* parent = 0);

    void setZoomStepTime(const quint64& zoomStepTime);
    void setElementalZoomTime(const quint64& elementalZoomTime);

    quint64 getDefaultScale() const;
    quint64 getZoomStepTime() const;
    quint64 getElementalZoomTime() const;

    private slots:
    void onUpdateScale(qreal);                          // Elementary scale update when zooming
    void scalingFinished();

    public slots:
    void startScaling(const int& delta);                // delta = number of wheel steps
    void stopScaling();

signals:
    void scale(qreal factor);
};

//////////////////////////////////////////////////////////////////////////////
///////////////          SphereTimeLineScroller         //////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* Calculates time line scrolling
*/

class SphereTimeLineScroller : public QObject
{
    Q_OBJECT

    QTimeLine* mScrollingTimeLine;                        // Updates the current position when scrolling
    bool mDragIsOngoing;

    int mMouseDragDistance;                               // Mouse move distance with left button being pressed
    QDateTime mLastMouseTrack;                            // Moving start time
    double mInitialVelocity;                              // Scrolling start speed
    double mFrictionCoeff;
    double mMsecPerPixel;                                 // Scale - msec/px
    QDateTime mScrollStartTime;                           // Scrolling start pos

    static const int mFreeFallAcceleration = 10;
    static const int mElementalScrollTime = 25;           // Elementary scrolling period - 25 msec
    static const int mTotalElementalScrollDuration = 350; // msec

public:
    SphereTimeLineScroller(QObject* parent = 0);

    //setters
    void setFrictionCoefficient(const double& coeff);
    void addScrollingDelta(const int& delta);
    void setLastMouseTrackTime(const QDateTime& time);
    void setDragIsOngoing(const bool& isOngoing);

    //getters
    double getFrictionCoefficient() const;
    bool dragIsOngoing() const;
    bool scalingIsOngoing() const;

    private slots:
    void onUpdateScroll(qreal);
    void onScrollFinished();

    public slots:
    void startScrolling(const QDateTime startTime, const double msecPerPixel);
    void stopScrolling();

signals:
    void scroll(QDateTime newCentralTime);
};

//////////////////////////////////////////////////////////////////////////////
///////////////                 TaskStyle               //////////////////////
//////////////////////////////////////////////////////////////////////////////

struct TaskStyle
{
    QBrush brush;                                        // Brush to paint a task and it's events
    QPen infoPen;		                                 // Icons pen
    QString infoIconPath;                                // Path to the task info icon. (in svg)

    TaskStyle(const QBrush& itemBrush, const QPen& itemPen, const QString iconPath) :
             brush(itemBrush),
             infoPen(itemPen),
             infoIconPath(iconPath){}
};

//////////////////////////////////////////////////////////////////////////////
///////////////             TimeLineWidget              //////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
* The timeline class itself
*/

class TimeLineWidget : public QGraphicsView
{
    Q_OBJECT

public:
    struct TimeLineStyle
    {
        TimeLineGrid::TimeLineGridStyle gridStyle;
        TimeLineItems::TimeLineItemsStyle itemsStyle;
    };

    struct  TimeLineSettings
    {
        TimeLineGrid::TimeLineGridSettings gridSettings;
        TimeLineItems::TimeLineItemsSettings itemsSettings;
    };

private:
    //graphics
    TimeLineGrid* mGrid;
    TimeLineItems* mItems;
    QLabel* mTaskInfoLabel;                              // An informational widget showed when the mouse is pointed to a timeline item */
    QGraphicsProxyWidget* mRealTimeButtonProxy;
    SphereTimeLineScaler* mScaler;
    SphereTimeLineScroller* mScroller;

    //timing
    QTimer* mUpdateTimer;                                // Updates timeline every second

private:
    void rearrangeWidgets(QSize size);
    QString createStringForItem(TimeLineItemPtr ptr);     // Creates a text for mTaskInfoLabel

public:
    explicit TimeLineWidget(TaskStoragePtr tasks, QWidget* parent = 0);

    void setStyle(const TimeLineStyle& style);
    void setSettings(const TimeLineSettings& settings);

    TimeLineStyle getStyle() const;
    TimeLineSettings getSettings() const;

    public slots:
    void addItemType(const TimeLineTaskType type, const TaskStyle& style);
    void setScale(qreal factor);                          // Change timeline scale. Calculated as follows: current time delta / factor
    void setCentralTime(QDateTime time);                  // Central time mark change

    private slots:
    void onUpdateTimeLine();                               // Called by mUpdateTimer
    void setRealTime();

protected:
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent * event);
    void resizeEvent(QResizeEvent * event);

signals:
    void eventClicked(quint64 taskId, QDateTime startTime);  // Item selection signal
    void rangeChanged(QDateTime startTime, QDateTime endTime);
};

#endif // TIMELINE_H
