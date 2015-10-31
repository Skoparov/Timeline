#include "timeline.h"

//////////////////////////////////////////////////////////////////////////////
///////////////             AbstractTimeLineItem         /////////////////////
//////////////////////////////////////////////////////////////////////////////

AbstractItem::AbstractItem(QDateTime startTime, QDateTime endTime) :
mStartTime(startTime),
mEndTime(endTime)
{

}

void AbstractItem::setStartTime(const QDateTime startTime)
{
    mStartTime = startTime;
}

void AbstractItem::setEndTime(const QDateTime endTime)
{
    mEndTime = endTime;
}

QDateTime AbstractItem::getStartTime() const
{
    return mStartTime;
}

QDateTime AbstractItem::getEndTime() const
{
    return mEndTime;
}

QPair <QDateTime, QDateTime> AbstractItem::getIntersection(const QDateTime& startTime, const QDateTime& endTime) const
{
    QPair<QDateTime, QDateTime> intersection;

    if (startTime < endTime){
        intersection = QPair <QDateTime, QDateTime>(std::max(startTime, mStartTime), std::min(endTime, mEndTime));
    }

    // If there is no intersection, invalidate result
    if (intersection.first >= intersection.second){
        intersection = QPair <QDateTime, QDateTime>();
    }

    return intersection;
}

//////////////////////////////////////////////////////////////////////////////
///////////////                TaskItem                      /////////////////////
//////////////////////////////////////////////////////////////////////////////

TaskItem::TaskItem(QDateTime startTime, QDateTime endTime, quint64 taskId, bool isInfinite, TimeLineTaskType taskType) :
mTaskId(taskId),
mIsInfinite(isInfinite),
mTaskType(taskType),
AbstractItem(startTime, endTime)
{
    if (!mEndTime.isValid()){
        mEndTime = isInfinite ? QDateTime::currentDateTime().addYears(100) : mStartTime;
    }
}

bool TaskItem::addEvent(EventItemPtr event)
{
    if (event == nullptr){
        return false;
    }

    if (!mEvent.contains(event->getStartTime()))
    {
        mEvent.insert(event->getEndTime(), event);
        if (event->getStatus() == EventItem::EVENT_STATUS_FAILED)
        {
            QDateTime failureTime = QDateTime::fromMSecsSinceEpoch(event->getStartTime().toMSecsSinceEpoch() / 2 +
                event->getEndTime().toMSecsSinceEpoch() / 2);

            mEventsWithInfoSigh.insert(failureTime, event);
        }

        if (!mIsInfinite && (mEndTime < event->getEndTime() || !mEndTime.isValid())){
            mEndTime = event->getEndTime();
        }
    }

    return true;
}

bool TaskItem::isInfinite() const
{
    return mIsInfinite;
}

quint32 TaskItem::eventCount() const
{
    return mEvent.size();
}

const QMap<QDateTime, EventItemPtr> &TaskItem::getEvents() const
{
    return mEvent;
}

const QMap<QDateTime, EventItemPtr> &TaskItem::getEventsWithInfoIcon() const
{
    return mEventsWithInfoSigh;
}

quint64 TaskItem::getTaskId() const
{
    return mTaskId;
}

TimeLineTaskType TaskItem::getTaskType() const
{
    return mTaskType;
}

AbstractItem::ItemType TaskItem::getItemType() const
{
    return ITEM_TYPE_TASK;
}

//////////////////////////////////////////////////////////////////////////////
///////////////                EventItem                 /////////////////////
//////////////////////////////////////////////////////////////////////////////

EventItem::EventItem(QDateTime startTime, QDateTime endTime, EventStatus stat) :
                    AbstractItem(startTime, endTime),
                    mStatus(stat)
{

}

bool EventItem::setParentTask(TaskItemPtr task)
{
    if (task == nullptr){
        return false;
    }

    mParentTask = task;
    return true;
}

const TaskItemPtr EventItem::getParentTask() const
{
    return mParentTask;
}

EventItem::EventStatus EventItem::getStatus() const
{
    return mStatus;
}

AbstractItem::ItemType EventItem::getItemType() const
{
    return ITEM_TYPE_EVENT;
}

//////////////////////////////////////////////////////////////////////////////
///////////////				TimeLineGrid				//////////////////////
//////////////////////////////////////////////////////////////////////////////

TimeLineGrid::TimeLineGrid(QGraphicsItem *parent) : QGraphicsItem(parent)
{

}

void TimeLineGrid::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    //debug info
    //painter->drawText(mBorderIndentX + 10, mSize.height() - 30 - mBorderIndentY, "BeginTime : " + mTimeCenterMark.addMSecs(-1 * (quint64)mTimeDelta).toString());
    //painter->drawText(mBorderIndentX + 10, mSize.height() - 15 - mBorderIndentY, "EndTime : " + mTimeCenterMark.addMSecs((quint64)mTimeDelta).toString());
    //painter->drawText(mBorderIndentX + 10, mSize.height() - 45 - mBorderIndentY, "MousePos : " + QString::number(mMousePos.x()));

    //-----------------------------------------------
    painter->setPen(QPen(mStyle.borderColor));
    // Paint item area border
    painter->drawRect(graphicsRect());

    // Paint central mark and mouse mark and corresponding texts
    if (mTimeCenterMark.isValid()){
        drawMarks(painter);
    }
}

void TimeLineGrid::drawMarks(QPainter *painter)
{
    quint64 startTime = mTimeCenterMark.toMSecsSinceEpoch() - mTimeDelta;
    quint64 endTime = mTimeCenterMark.toMSecsSinceEpoch() + mTimeDelta;
    quint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    double msecPerPixel = (2 * mTimeDelta) / mSize.width();
    double pixelsPerMsec = 1 / msecPerPixel;

    QFont font = painter->font();
    QFontMetrics fm(font);
    QString textFormat = 2 * mTimeDelta < day ? "hh:mm:ss" : "dd.MM.yy";
    font.setPointSize(mSettings.borderIndentY * 0.5);

    int triangleRectWidth = (mSettings.borderIndentY - fm.height()) / 2 + 1;

    // Current time mark and it's text
    QPair<int, int> currTimeMarkBorders(-1, -1);
    QString currMarkTimeString = mTimeCenterMark.toString(textFormat);
    quint16 currTimeMarkWidth = fm.width(currMarkTimeString);
    quint64 currTimeMarkWidthMsec = currTimeMarkWidth*msecPerPixel;

    QPair<quint64, quint64> intersection(std::max(currTime - currTimeMarkWidthMsec, startTime),
                                     std::min(currTime + currTimeMarkWidthMsec, endTime));

    if (intersection.first < intersection.second)
    {
        painter->setPen(QPen(mStyle.currMarkColor));

        double pixelsPerMsec = 1/msecPerPixel;
        int currTimePos = (currTime > startTime) ?
            pixelsPerMsec * (currTime - startTime) : -pixelsPerMsec * (startTime - currTime);

        currTimeMarkBorders.first = currTimePos - currTimeMarkWidth;
        currTimeMarkBorders.second = currTimePos + currTimeMarkWidth;

        // line
        if (currTimePos < mSize.width() - mSettings.borderIndentX &&  currTimePos > mSettings.borderIndentX){
            painter->drawLine(currTimePos, mSettings.borderIndentY, currTimePos, mSize.height() - mSettings.borderIndentY);
        }

        // curr time mark text, the size is calculated depending on the indent from the border
        paintText(true, currTimePos, currMarkTimeString, painter, mStyle.currMarkColor);
    }

    // grid time marks
    painter->setPen(QPen(mStyle.timeMarksTextColor));

    // calculate step between grid items in msec
    double part = mMousePos.x() / mSize.width();
    quint64 mSecSinseStart = 2 * mTimeDelta*part + mTimeCenterMark.addMSecs(-1 * (quint64)mTimeDelta).toMSecsSinceEpoch();
    QString mouseTimeString = QDateTime().fromMSecsSinceEpoch(mSecSinseStart).toString("dd.MM.yy hh:mm:ss");
    quint16 textWidth = fm.width(mouseTimeString);
    quint16 maxNumberOfTextMarks = mSize.width() / (textWidth*1.5);
    if (maxNumberOfTextMarks == 0){
        return;
    }

    quint64 stepMsec = calculateStep(maxNumberOfTextMarks);

    // find first item
    quint64 timeMark = startTime;
    if (startTime % stepMsec){
        timeMark = (startTime / stepMsec - 1)*stepMsec;
    }

    while (true)
    {
        // to make text appear smoothly
        int pos = timeMark > startTime ?
            pixelsPerMsec * (timeMark - startTime) : -pixelsPerMsec * (startTime - timeMark);

        QString timeText = QDateTime().fromMSecsSinceEpoch(timeMark).toString(textFormat);

        if (pos - fm.width(timeText) / 2 < mSize.width() - mSettings.borderIndentX)
        {
            // make an item semitransparent when it overlays the current mark text
            qreal opacity = 1;
            if (currTimeMarkBorders.first != -1 && currTimeMarkBorders.second != -1)
            {
                QPair<int, int> intersection(std::max(currTimeMarkBorders.first, pos - fm.width(timeText) / 2),
                    std::min(currTimeMarkBorders.second, pos + fm.width(timeText) / 2));

                if (intersection.first < intersection.second){
                    opacity = 0.3;
                }
            }

            painter->setOpacity(opacity);
            painter->drawLine(pos, mSettings.borderIndentY, pos, mSettings.borderIndentY - triangleRectWidth);
            paintText(true, pos, timeText, painter, mStyle.timeMarksTextColor);

            timeMark += stepMsec;
        }
        else{
            break;
        }
    }

}

quint64 TimeLineGrid::calculateStep(const int& maxNumberOfTextMarks)
{
    quint64 stepMsec = 2 * mTimeDelta / maxNumberOfTextMarks;
    const quint64 month = (quint64)day * 30;
    const quint64 year = (quint64)day * 365;

    if (stepMsec < second){
        return quint64(-1);
    }

    quint64 timeUnit = second;
    quint64 tempStepMsec = stepMsec;

    while (true)
    {
        if (tempStepMsec >= minute &&  tempStepMsec < hour){
            timeUnit = minute;
        }
        else if (tempStepMsec >= hour &&  tempStepMsec < day){
            timeUnit = hour;
        }
        else if (tempStepMsec >= day &&  tempStepMsec < month){
            timeUnit = day;
        }
        else if (tempStepMsec >= month && tempStepMsec < year){
            timeUnit = month;
        }
        else if (tempStepMsec >= year){
            timeUnit = year;
        }

        int numberOfUnits = tempStepMsec / timeUnit;
        if (numberOfUnits > 2){
            numberOfUnits = 5 * std::ceil((double)numberOfUnits / 5);
        }

        tempStepMsec = numberOfUnits * timeUnit;
        int currNumberOfTimeMarks = 2 * mTimeDelta / tempStepMsec;

        if (currNumberOfTimeMarks > maxNumberOfTextMarks) //minutes or seconds
        {
            switch (timeUnit)
            {
            case second: tempStepMsec += second; break;
            case minute: tempStepMsec += minute; break;
            case hour: tempStepMsec += hour; break;
            case day: tempStepMsec += day; break;
            case month: tempStepMsec = year; break;
            case year: tempStepMsec += year; break;
            default: break;
            }

            continue;
        }

        if ((timeUnit == second || timeUnit == minute) && numberOfUnits > 30)
        {
            tempStepMsec = 60 * timeUnit;
            continue;
        }

        if (2 * mTimeDelta > day && timeUnit < day){
            tempStepMsec = day;
        }

        stepMsec = tempStepMsec;
        break;
    }

    return stepMsec;
}

void TimeLineGrid::paintText(bool topBottom, int xPos, QString text, QPainter* painter, QColor color)
{
    QPen pen = painter->pen();
    QFont font = painter->font();
    QFontMetrics fm(font);

    pen.setColor(color);
    painter->setPen(pen);

    int yPos = topBottom ? (mSettings.borderIndentY - fm.height()) / 2 : mSize.height() - fm.height() - (mSettings.borderIndentY - fm.height()) / 2;
    QRect textRect(xPos - fm.width(text) / 2, yPos, fm.width(text), fm.height());
    painter->drawText(textRect, Qt::AlignCenter, text);
}

bool TimeLineGrid::setTimeRange(const QDateTime& centralTime, const quint64& timeDelta)
{
    /**< If the new scale is valid, set it */
    if (timeDelta >= mSettings.maximumScale && timeDelta <= mSettings.minimumScale)
    {
        mTimeCenterMark = centralTime;
        mTimeDelta = timeDelta;
        update();

        return true;

        emit rangeChanged(mTimeCenterMark.addMSecs((-1)*timeDelta), mTimeCenterMark.addMSecs(timeDelta));
    }

    return false;
}

void TimeLineGrid::setStyle(const TimeLineGridStyle& style)
{
    mStyle = style;
}

void TimeLineGrid::setSettings(const TimeLineGridSettings& settings)
{
    mSettings = settings;
}

void TimeLineGrid::setMousePos(const QPoint &pos, bool isDragging)
{
    if (isDragging)
    {
        int mouseDelta = mMousePos.x() - pos.x();
        if (mouseDelta != 0)
        {
            int sign = mouseDelta / std::abs(mouseDelta);
            quint64 deltaMSec = 2 * mTimeDelta*std::abs(mouseDelta / mSize.width());
            mTimeCenterMark = mTimeCenterMark.addMSecs(sign*deltaMSec);
        }
    }

    mMousePos = pos;
    update();
}

void TimeLineGrid::setSize(const QSizeF &size, const QPointF& pos)
{
    mSize = size;
    setPos(pos);
}

QDateTime TimeLineGrid::getTimeMark() const
{
    return mTimeCenterMark;
}

quint64 TimeLineGrid::getTimeDelta() const
{
    return mTimeDelta;
}

QPoint TimeLineGrid::getMousePos() const
{
    return mMousePos;
}

TimeLineGrid::TimeLineGridSettings TimeLineGrid::getSettings() const
{
    return mSettings;
}

TimeLineGrid::TimeLineGridStyle TimeLineGrid::getStyle() const
{
    return mStyle;
}

QRectF TimeLineGrid::boundingRect() const
{
    return QRectF(0, 0, mSize.width(), mSize.height());
}

QRect TimeLineGrid::graphicsRect() const
{
    // Timeline item painting area, with indents taken into account
    return QRect(mSettings.borderIndentX, mSettings.borderIndentY, mSize.width() - 2 * mSettings.borderIndentX, mSize.height() - 2 * mSettings.borderIndentY);
}

//////////////////////////////////////////////////////////////////////////////
///////////////				TaskStorage					//////////////////////
//////////////////////////////////////////////////////////////////////////////

bool TaskStorage::addTask(const TaskItemPtr task)
{
    QMutexLocker lock(&mMutex);

    if (task == nullptr){
        return false;
    }

    auto taskIter = mTasks.find(task->getTaskId());
    if (taskIter == mTasks.end() || *taskIter == nullptr){
        mTasks.insert(task->getTaskId(), task);
    }
    else
    {
        auto existingTask = *taskIter;
        if (existingTask->getEndTime() != task->getEndTime()){
            existingTask->setEndTime(task->getEndTime());
        }
    }

    return true;
}

void TaskStorage::removeTask(const quint64& taskId)
{
    QMutexLocker lock(&mMutex);

    auto taskIter = mTasks.find(taskId);
    if (taskIter != mTasks.end() && (*taskIter) != nullptr)
    {
        TaskItemPtr taskPtr = *taskIter;
        bool noNeedToDelete = taskPtr->eventCount();

        if (!noNeedToDelete){
            mTasks.remove(taskId);
        }
    }
}

bool TaskStorage::addEvent(const quint32 taskId, const EventItemPtr event)
{
    QMutexLocker lock(&mMutex);
    bool result = true;

    auto parentTask = mTasks.find(taskId);
    if (parentTask == mTasks.end()){
        result = false;
    }

    if (result && (*parentTask)->addEvent(event)){
        event->setParentTask(*parentTask);
    }
    else{
        result = false;
    }

    return result;
}

void TaskStorage::clear()
{
    QMutexLocker lock(&mMutex);

    mTasks.clear();
}

TaskItemPtr TaskStorage::getTask(const quint64& taskId)
{
    QMutexLocker lock(&mMutex);

    TaskItemPtr taskPtr;

    auto taskIter = mTasks.find(taskId);
    if (taskIter != mTasks.end()){
        taskPtr = *taskIter;
    }

    return taskPtr;
}

EventItemPtr TaskStorage::getEvent(const quint64& taskId, const QDateTime& startTime)
{
    QMutexLocker lock(&mMutex);

    EventItemPtr eventPtr;

    auto taskIter = mTasks.find(taskId);
    if (taskIter != mTasks.end())
    {
        TaskItemPtr taskPtr = *taskIter;
        const QMap<QDateTime, EventItemPtr> events = taskPtr->getEvents();
        auto eventIter = events.find(startTime);
        if (eventIter != events.end()){
            eventPtr = *eventIter;
        }
    }

    return eventPtr;
}

const QHash<quint64, TaskItemPtr> TaskStorage::getTasks()
{
    return mTasks;
}

void TaskStorage::lock()
{
    mMutex.lock();
}

void TaskStorage::unlock()
{
    mMutex.unlock();
}

//////////////////////////////////////////////////////////////////////////////
///////////////				TimeLineItems				//////////////////////
//////////////////////////////////////////////////////////////////////////////

TimeLineItems::TimeLineItems(TaskStoragePtr tasks, QGraphicsItem *parent) : mTaskStorage(tasks), QGraphicsItem(parent)
{

}

void TimeLineItems::setSize(const QSizeF &size, const QPointF &pos)
{
    mSize = size;
    setPos(pos);
    update();
}

void TimeLineItems::setTime(const QDateTime& centralTime, const quint64& timeDelta)
{
    mCentralTime = centralTime;
    mTimeDelta = timeDelta;
    update();
}

void TimeLineItems::setSelectedItem(const TimeLineItemPtr item)
{
    mSelectedItem = item;
    update();
}

void TimeLineItems::addItemType(const TimeLineTaskType type, const TaskStyle& style)
{
    TaskStylePtr stylePtr = std::make_shared<TaskStyle>(style.brush, style.infoPen, style.infoIconPath);
    mItemStyles.insert(type, stylePtr);
}

void TimeLineItems::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    calculateVisibleItems();

    painter->fillRect(boundingRect(), QBrush(mStyle.backgroundColor));

    painter->setPen(QPen(mStyle.borderColor));
    quint16 resultAreaHeight = mSize.height()*mSettings.infoHeightPortion;
    painter->drawLine(1, resultAreaHeight, mSize.width(), resultAreaHeight);

    // paint axis
    QColor axisColor = mStyle.borderColor;
    axisColor.setAlphaF(mStyle.axisOpacity);
    painter->setPen(QPen(axisColor));
    quint32 distBetweenAxis = (boundingRect().height() - resultAreaHeight) / (mItemStyles.size() + 1);
    for (quint8 axisNum = 0; axisNum < mItemStyles.size(); ++axisNum)
    {
        quint32 currAxisYPos = boundingRect().height() - distBetweenAxis * (axisNum + 1);
        painter->drawLine(1, currAxisYPos, mSize.width(), currAxisYPos);
    }

    // Paint visible items
    for (auto& visibleItem : mVisibleItems)
    {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::HighQualityAntialiasing);

        QRect rect = visibleItem.rect;

        // Get brush for the item
        QBrush brush = visibleItem.style->brush;
        if (visibleItem.item == mSelectedItem){
            brush.setColor(mStyle.selectedItemColor);
        }

        AbstractItem::ItemType type = visibleItem.item->getItemType();
        if (type == AbstractItem::ITEM_TYPE_EVENT){
            painter->setOpacity(mStyle.eventPaintOpacity);
        }
        else if (type == AbstractItem::ITEM_TYPE_TASK){
            painter->setOpacity(mStyle.taskPaintOpacity);
        }

        // Draw and fill the item
        if (brush.color() != mStyle.selectedItemColor){
            painter->setPen(QPen(brush.color()));
        }

        QPainterPath rectPath;
        rectPath.addRoundedRect(rect, rect.height() / 4, rect.height() / 4);
        painter->fillPath(rectPath, brush);
        painter->drawPath(rectPath);

        painter->setOpacity(1);
    }


    // paint icons
    if (!mInfoMarks.isEmpty())
    {
        quint16 warningSignMinWidth = resultAreaHeight;
        quint16 maxWarningSigns = 8 * mSize.width() / resultAreaHeight;

        quint16 warningLineStart_Y = mInfoMarks.size() <= maxWarningSigns ? resultAreaHeight / 2 : 0;

        QHash<QString, std::shared_ptr<QSvgRenderer>> svgRenderers;
        QImage image(warningSignMinWidth, warningSignMinWidth, QImage::Format_ARGB32);
        image.fill(0);
        QPainter painter1(&image);

        for (auto mark = mInfoMarks.begin(); mark != mInfoMarks.end(); ++mark)
        {
            auto markStyle = *mark;
            auto markPos = mark.key();

            if (markStyle->infoIconPath.isEmpty()){
                continue;
            }

            painter->setPen(markStyle->infoPen);
            if (markPos < mSize.width())
            {
                painter->setRenderHints(QPainter::Antialiasing, false);
                painter->setRenderHints(QPainter::HighQualityAntialiasing, false);
                painter->drawLine(markPos, warningLineStart_Y, markPos, resultAreaHeight - 1);
            }

            if (mInfoMarks.size() <= maxWarningSigns)
            {
                std::shared_ptr<QSvgRenderer> renderer;

                auto rendererIter = svgRenderers.find(markStyle->infoIconPath);
                if (rendererIter == svgRenderers.end())
                {
                    renderer = std::make_shared<QSvgRenderer>(markStyle->infoIconPath);
                    renderer->render(&painter1);
                    svgRenderers.insert(markStyle->infoIconPath, renderer);

                }
                else{
                    renderer = *rendererIter;
                }

                QRectF sourseRect(0.0, 0.0, warningSignMinWidth, warningSignMinWidth);
                QRectF imageRect(markPos - warningSignMinWidth / 2, (resultAreaHeight - warningSignMinWidth) / 2, warningSignMinWidth, warningSignMinWidth);

                if (imageRect.left() < 0)
                {
                    sourseRect.setLeft(std::abs(imageRect.left()));
                    imageRect.setLeft(0);
                }

                if (imageRect.right() > mSize.width())
                {
                    int delta = imageRect.right() - mSize.width();
                    sourseRect.setRight(sourseRect.right() - delta);
                    imageRect.setRight(imageRect.right() - delta);
                }

                painter->setRenderHints(QPainter::Antialiasing, true);
                painter->setRenderHints(QPainter::HighQualityAntialiasing, true);
                painter->drawImage(imageRect, image, sourseRect);
            }
        }
    }
}

void TimeLineItems::calculateVisibleItems()
{
    if (mTaskStorage == nullptr || mItemStyles.isEmpty()){
        return;
    }

    QDateTime visibleRangeStartTime = mCentralTime.addMSecs(-1 * (quint64)mTimeDelta);
    QDateTime visibleRangeEndTime = mCentralTime.addMSecs(mTimeDelta);
    double pixelsPerMSec = (double)mSize.width() / (2 * mTimeDelta);
    quint16 resultAreaHeight = mSize.height()*mSettings.infoHeightPortion;

    mVisibleItems.clear();
    mInfoMarks.clear();

    mTaskStorage->lock();

    quint32 distBetweenAxis = (boundingRect().height() - resultAreaHeight) / (mItemStyles.size() + 1);
    quint32 taskHeight = distBetweenAxis * mSettings.taskHeightPortion;
    quint32 eventHeight = distBetweenAxis * mSettings.eventsHeightPortion;

    for (auto task : mTaskStorage->getTasks())
    {
        // The task  has not specified end time and no events
        if (!task->eventCount() &&
            !task->getEndTime().isValid()){
            continue;
        }

        // Check if there is and axis for the task
        auto currItemStylePtr = mItemStyles.find(task->getTaskType());
        if (currItemStylePtr == mItemStyles.end()){
            continue;
        }

        quint32 currAxisConsecNumber = std::distance(mItemStyles.begin(), currItemStylePtr);
        quint32 currAxisYPos = boundingRect().height() - distBetweenAxis * (currAxisConsecNumber + 1);

        QPair<QDateTime, QDateTime> intersection = task->getIntersection(visibleRangeStartTime, visibleRangeEndTime);
        if (!intersection.first.isValid()){
            continue;
        }

        /**< Task itself */
        quint32 startPos = (intersection.first.toMSecsSinceEpoch() - visibleRangeStartTime.toMSecsSinceEpoch())*pixelsPerMSec;
        quint32 width = (intersection.second.toMSecsSinceEpoch() - intersection.first.toMSecsSinceEpoch())*pixelsPerMSec;
        QRect itemRect(startPos, currAxisYPos - taskHeight / 2, width, taskHeight);

        mVisibleItems.append(VisibleItem(task, *currItemStylePtr, itemRect));

        /**< If the scale is appropriate, paint events */
        if (mTimeDelta <= mSettings.eventsVisibleScale && task->eventCount())
        {
            const QMap<QDateTime, EventItemPtr>& events = task->getEvents();

            auto event = events.lowerBound(visibleRangeStartTime);
            if (event != events.end() && (*event)->getEndTime() == visibleRangeStartTime){
                event++;
            }

            for (; event != events.end() && (*event)->getStartTime() < visibleRangeEndTime; ++event)
            {
                QPair<QDateTime, QDateTime> intersection = (*event)->getIntersection(visibleRangeStartTime, visibleRangeEndTime);

                //rect
                quint32 startPosX = (intersection.first.toMSecsSinceEpoch() - visibleRangeStartTime.toMSecsSinceEpoch())*pixelsPerMSec;
                quint32 width = (intersection.second.toMSecsSinceEpoch() - intersection.first.toMSecsSinceEpoch())*pixelsPerMSec;

                QRect itemRect(startPosX, currAxisYPos - eventHeight / 2, width, eventHeight);
                mVisibleItems.append(VisibleItem(*event, *currItemStylePtr, itemRect));
            }
        }

        //info marks
        const QMap<QDateTime, EventItemPtr>& eventsWithInfoIcons = task->getEventsWithInfoIcon();
        auto event = eventsWithInfoIcons.lowerBound(visibleRangeStartTime);

        for (; event != eventsWithInfoIcons.end() && event.key() < visibleRangeEndTime; ++event)
        {
            int pos = (event.key().toMSecsSinceEpoch() - visibleRangeStartTime.toMSecsSinceEpoch())*pixelsPerMSec;
            mInfoMarks.insert(pos, *currItemStylePtr);
        }
    }

    mTaskStorage->unlock();
}

QList<TimeLineItemPtr> TimeLineItems::getItemUnderPos(QPoint &pos)
{
    QList<TimeLineItemPtr> result;

    for (auto visibleItem = mVisibleItems.begin(); visibleItem != mVisibleItems.end(); ++visibleItem)
    {
        if (visibleItem->rect.contains(pos.x() - this->pos().x(), pos.y() - this->pos().y(), true)){
            result.append(visibleItem->item);
        }
    }

    return result;
}

void TimeLineItems::setSettings(const TimeLineItemsSettings& settings)
{
    mSettings = settings;
}

void TimeLineItems::setStyle(const TimeLineItemsStyle& style)
{
    mStyle = style;
}

TimeLineItems::TimeLineItemsSettings TimeLineItems::getSettings() const
{
    return mSettings;
}

TimeLineItems::TimeLineItemsStyle TimeLineItems::getStyle() const
{
    return mStyle;
}

QRectF TimeLineItems::boundingRect() const
{
    return QRectF(QPointF(0, 0), QPointF(mSize.width(), mSize.height()));
}

//////////////////////////////////////////////////////////////////////////////
///////////////			SphereTimeLineScaler			//////////////////////
//////////////////////////////////////////////////////////////////////////////

SphereTimeLineScaler::SphereTimeLineScaler(QObject* parent /*= 0*/) :
mZoomStepTime(350),
mElementalZoomTime(50),
mZoomStepRelaxationCoeff(0.000625),
mScheduledScaling(0),
QObject(parent)
{
    mZoomingTimeLine = new QTimeLine(mZoomStepTime, this);
    mZoomingTimeLine->setUpdateInterval(mElementalZoomTime);

    connect(mZoomingTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(onUpdateScale(qreal)));
}

void SphereTimeLineScaler::startScaling(const int& delta)
{
    int stepNum = delta / 120; // One wheel step = 120 units (or 15 degrees)
    mScheduledScaling += stepNum;

    if (mScheduledScaling * stepNum < 0){ // // If zooming direction has changed, drop all planned zooming iterations
        mScheduledScaling = stepNum;
    }

    if (mZoomingTimeLine->state() == QTimeLine::NotRunning){
        mZoomingTimeLine->start();
    }
}

void SphereTimeLineScaler::stopScaling()
{
    mZoomingTimeLine->stop();
}

void SphereTimeLineScaler::onUpdateScale(qreal)
{
    qreal factor = 1.0 + qreal(mScheduledScaling)*mZoomStepRelaxationCoeff; // The more zomoming iterations are scheduled, the faster the zooming is
    emit scale(factor);
}

void SphereTimeLineScaler::scalingFinished()
{
    if (mScheduledScaling > 0){
        mScheduledScaling--;
    }
    else{
        mScheduledScaling++;
    }
}

void SphereTimeLineScaler::setZoomStepTime(const quint64& zoomStepTime)
{
    if (zoomStepTime > 0)
    {
        mZoomStepTime = zoomStepTime;
        mZoomingTimeLine->setDuration(mZoomStepTime);
    }
}

void SphereTimeLineScaler::setElementalZoomTime(const quint64& elementalZoomTime)
{
    mElementalZoomTime = elementalZoomTime;
    mZoomingTimeLine->setUpdateInterval(50);
}

quint64 SphereTimeLineScaler::getZoomStepTime() const
{
    return mZoomingTimeLine->duration();
}

quint64 SphereTimeLineScaler::getElementalZoomTime() const
{
    return mZoomingTimeLine->updateInterval();
}

quint64 SphereTimeLineScaler::getDefaultScale() const
{
    return mDefaultScale;
}

//////////////////////////////////////////////////////////////////////////////
///////////////			SphereTimeLineScroller			//////////////////////
//////////////////////////////////////////////////////////////////////////////


SphereTimeLineScroller::SphereTimeLineScroller(QObject* parent /*= 0*/) :
mDragIsOngoing(false),
mMouseDragDistance(0),
mInitialVelocity(0),
mMsecPerPixel(0),
mFrictionCoeff(0.66),
QObject(parent)
{
    mScrollingTimeLine = new QTimeLine(350, this);
    mScrollingTimeLine->setUpdateInterval(mElementalScrollTime);

    connect(mScrollingTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(onUpdateScroll(qreal)));
}

void SphereTimeLineScroller::startScrolling(const QDateTime startTime, const double msecPerPixel)
{
    // If there was a move at all
    if (mMouseDragDistance)
    {
        // Start speed in px/msec, where 1 px = 1 millimeter
        mInitialVelocity = (double)mMouseDragDistance / (QDateTime::currentDateTime().toMSecsSinceEpoch() - mLastMouseTrack.toMSecsSinceEpoch());
        double scrollTime = std::abs(mInitialVelocity / (mFrictionCoeff*mFreeFallAcceleration)) * 1000; // The time until the scrolling stops

        if (scrollTime > 0)
        {
            mMsecPerPixel = msecPerPixel;
            mScrollStartTime = startTime;

            mScrollingTimeLine->setDuration(scrollTime);
            mScrollingTimeLine->start();
        }
    }
}

void SphereTimeLineScroller::stopScrolling()
{
    mInitialVelocity = 0;
    mMouseDragDistance = 0;

    if (mScrollingTimeLine->state() == QTimeLine::Running){
        mScrollingTimeLine->stop();
    }
}

void SphereTimeLineScroller::onScrollFinished()
{
    mInitialVelocity = 0;
    mMouseDragDistance = 0;
}

void SphereTimeLineScroller::onUpdateScroll(qreal)
{
    if (mScrollingTimeLine->state() == QTimeLine::NotRunning){
        return;
    }

    int elapsedTime = mScrollingTimeLine->currentTime();
    double acceleration = mFrictionCoeff*mFreeFallAcceleration / 1000; //  in m/(sec^2)

    if (mInitialVelocity > 0){ //direction
        acceleration = -acceleration;
    }

    double newPos = mInitialVelocity*elapsedTime + acceleration*elapsedTime*elapsedTime / 2; // Distance from the start: v0*t - (a*t^2)/2
    QDateTime newCentralTime = mScrollStartTime.addMSecs(newPos*mMsecPerPixel); // New central time

    emit scroll(newCentralTime);
}

void SphereTimeLineScroller::addScrollingDelta(const int& delta)
{
    mMouseDragDistance += delta;

    if (mMouseDragDistance * delta < 0)
    {
        mMouseDragDistance = delta;
        mLastMouseTrack = QDateTime::currentDateTime();
    }
}

void SphereTimeLineScroller::setLastMouseTrackTime(const QDateTime& time)
{
    mLastMouseTrack = time;
}

void SphereTimeLineScroller::setFrictionCoefficient(const double& coeff)
{
    mFrictionCoeff = coeff;
}

void SphereTimeLineScroller::setDragIsOngoing(const bool& isOngoing)
{
    mDragIsOngoing = isOngoing;
}


bool SphereTimeLineScroller::scalingIsOngoing() const
{
    return mScrollingTimeLine->state() == QTimeLine::Running;
}

double SphereTimeLineScroller::getFrictionCoefficient() const
{
    return mFrictionCoeff;
}

bool SphereTimeLineScroller::dragIsOngoing() const
{
    return mDragIsOngoing;
}


//////////////////////////////////////////////////////////////////////////////
///////////////                TimeLineView             //////////////////////
//////////////////////////////////////////////////////////////////////////////

TimeLineWidget::TimeLineWidget(TaskStoragePtr tasks, QWidget *parent) : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // Scaling and scrolling
    mScaler = new SphereTimeLineScaler(this);
    mScroller = new SphereTimeLineScroller(this);

    // Interface
    mGrid = new TimeLineGrid();
    mGrid->setTimeRange(QDateTime::currentDateTime(), mScaler->getDefaultScale());
    mGrid->setZValue(1);

    // Items
    mItems = new TimeLineItems(tasks);
    mItems->setZValue(0);

    // Info about an item
    mTaskInfoLabel = new QLabel(this, Qt::Popup);
    mTaskInfoLabel->setAutoFillBackground(true);
    mTaskInfoLabel->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    mTaskInfoLabel->setAttribute(Qt::WA_ShowWithoutActivating);
    QFont font = mTaskInfoLabel->font();
    font.setPixelSize(15);
    mTaskInfoLabel->setFont(font);
    mTaskInfoLabel->hide();

    // Real time button
    QPushButton* realTimeButton = new QPushButton();
    //realTimeButton->setFlat(true);
    realTimeButton->setCheckable(true);
    realTimeButton->setAttribute(Qt::WA_TranslucentBackground);
    realTimeButton->setStyleSheet("QPushButton {border-style: outset; border-width: 0px;}");

    // Timeline update
    mUpdateTimer = new QTimer(this);
    mUpdateTimer->start(1000);

    scene()->addItem(mGrid);
    scene()->addItem(mItems);
    mRealTimeButtonProxy = scene()->addWidget(realTimeButton);

    // Connections
    connect(mUpdateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimeLine()));
    connect(realTimeButton, SIGNAL(clicked()), this, SLOT(setRealTime()));
    connect(mScroller, SIGNAL(scroll(QDateTime)), this, SLOT(setCentralTime(QDateTime)));
    connect(mScaler, SIGNAL(scale(qreal)), this, SLOT(setScale(qreal)));
    connect(mGrid, SIGNAL(rangeChanged(QDateTime, QDateTime)), this, SIGNAL(rangeChanged(QDateTime, QDateTime)));

    viewport()->setCursor(Qt::OpenHandCursor);

    setRealTime();
}

void TimeLineWidget::mousePressEvent(QMouseEvent *event)
{
    // Used to select an element
    QPoint pos = event->pos();

    QList<TimeLineItemPtr> itemsUnderPos = mItems->getItemUnderPos(pos);

    // If there is only one element, and it's events
    if (itemsUnderPos.size())
    {
        for (auto item : itemsUnderPos)
        {
            if (item->getItemType() == AbstractItem::ITEM_TYPE_EVENT)
            {
                viewport()->setCursor(Qt::ArrowCursor);
                mItems->setSelectedItem(item);
                EventItemPtr event = std::dynamic_pointer_cast<EventItem>(item);
                emit eventClicked(event->getParentTask()->getTaskId(), event->getStartTime());

                qDebug() << QString("Clicked event: taskId %1 | startTime %2 | endTime %3")
                    .arg(event->getParentTask()->getTaskId())
                    .arg(event->getStartTime().toString())
                    .arg(event->getEndTime().toString());
            }
        }
    }
    else // otherwise scroll the timeline
    {
        if (rect().contains(event->pos()) && event->button() == Qt::LeftButton)
        {
            mScroller->stopScrolling();

            QPushButton* realTimeButton = (QPushButton*)mRealTimeButtonProxy->widget();
            if (!realTimeButton->isChecked()) // If real time mode is off, start scrolling
            {
                mScroller->setDragIsOngoing(true);
                mScroller->setLastMouseTrackTime(QDateTime::currentDateTime());
                viewport()->setCursor(Qt::ClosedHandCursor);

                if (mTaskInfoLabel->isVisible()){
                    mTaskInfoLabel->hide();
                }
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void TimeLineWidget::mouseMoveEvent(QMouseEvent *event)
{
    // If the cursor is out of widget's borders
    if (!mGrid->graphicsRect().contains(event->pos()))
    {
        mScroller->setDragIsOngoing(false);

        if (mTaskInfoLabel->isVisible()){
            mTaskInfoLabel->hide();
        }
    }

    QPoint pos = event->pos();

    QList<TimeLineItemPtr> itemsUnderPos = mItems->getItemUnderPos(pos);

    int prevMousePos = mGrid->getMousePos().x();
    mGrid->setMousePos(event->pos(), mScroller->dragIsOngoing());

    if (mScroller->dragIsOngoing())
    {
        int delta = prevMousePos - event->pos().x();
        mScroller->addScrollingDelta(delta); // Add new range to the scrolling path
        mItems->setTime(mGrid->getTimeMark(), mGrid->getTimeDelta());
    }
    else // paint pop up info about the task
    {
        QPoint pos = event->pos();
        QList<TimeLineItemPtr> itemsUnderPos = mItems->getItemUnderPos(pos);
        if (itemsUnderPos.size())
        {
            viewport()->setCursor(Qt::ArrowCursor);
            mTaskInfoLabel->move(pos.x(), pos.y() - mTaskInfoLabel->height()*1.5);
            mTaskInfoLabel->setText(createStringForItem(*(itemsUnderPos.end() - 1)));

            QFontMetrics fm(mTaskInfoLabel->font());
            int width = fm.width(mTaskInfoLabel->text());
            mTaskInfoLabel->setFixedWidth(width);

            if (!mTaskInfoLabel->isVisible()){
                mTaskInfoLabel->show();
            }
        }
        else if (mTaskInfoLabel->isVisible())
        {
            mTaskInfoLabel->hide();
            viewport()->setCursor(Qt::OpenHandCursor);
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

void TimeLineWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPoint pos = event->pos();
        QList<TimeLineItemPtr> itemsUnderPos = mItems->getItemUnderPos(pos);
        if (itemsUnderPos.isEmpty()){
            viewport()->setCursor(Qt::OpenHandCursor);
        }

        if (mScroller->dragIsOngoing()) // start scrolling
        {
            mScroller->setDragIsOngoing(false);
            double msecPerPx = (mGrid->getTimeDelta() * 2) / mGrid->graphicsRect().width();
            mScroller->startScrolling(mGrid->getTimeMark(), msecPerPx);
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void TimeLineWidget::wheelEvent(QWheelEvent *event)
{
    mScaler->startScaling(event->delta());

    QGraphicsView::wheelEvent(event);
}

QString TimeLineWidget::createStringForItem(TimeLineItemPtr item)
{
    TimeLineTaskType taskType;
    quint32 taskId;

    AbstractItem::ItemType type = item->getItemType();

    if (type == AbstractItem::ITEM_TYPE_EVENT)
    {
        EventItemPtr event = std::dynamic_pointer_cast<EventItem>(item);
        taskType = event->getParentTask()->getTaskType();
        taskId = event->getParentTask()->getTaskId();
    }
    else if (type == AbstractItem::ITEM_TYPE_TASK)
    {
        TaskItemPtr event = std::dynamic_pointer_cast<TaskItem>(item);
        taskType = event->getTaskType();
        taskId = event->getTaskId();
    }

    QString taskTypeStr;// = SphereConverter::taskTypeToString(taskType);

    return QString(" %1 #%2 ").arg(taskTypeStr).arg(taskId);
}

void TimeLineWidget::addItemType(const TimeLineTaskType type, const TaskStyle& style)
{
    mItems->addItemType(type, style);
}

void TimeLineWidget::setScale(qreal factor)
{
    int newDelta = mGrid->getTimeDelta() / factor;

    if (mGrid->setTimeRange(mGrid->getTimeMark(), newDelta)){
        mItems->setTime(mGrid->getTimeMark(), mGrid->getTimeDelta());
    }
    else{
        mScaler->stopScaling();
    }
}

void TimeLineWidget::setCentralTime(QDateTime time)
{
    if (mGrid->setTimeRange(time, mGrid->getTimeDelta())){
        mItems->setTime(mGrid->getTimeMark(), mGrid->getTimeDelta());
    }
}

void TimeLineWidget::onUpdateTimeLine()
{
    bool ok = mGrid->setTimeRange(mGrid->getTimeMark().addSecs(1), mGrid->getTimeDelta());

    if (ok){
        mItems->setTime(mGrid->getTimeMark(), mGrid->getTimeDelta());
    }
}

void TimeLineWidget::resizeEvent(QResizeEvent *event)
{
    rearrangeWidgets(event->size());
    QGraphicsView::resizeEvent(event);
}

void TimeLineWidget::rearrangeWidgets(QSize size)
{
    mGrid->setSize(size, QPointF(0, 0));

    QRect graphicsRect = mGrid->graphicsRect();
    mItems->setSize(graphicsRect.size(), graphicsRect.topLeft());
    mItems->setTime(mGrid->getTimeMark(), mGrid->getTimeDelta());

    QPushButton* realTimeButton = (QPushButton*)mRealTimeButtonProxy->widget();
    double sizeMult = 0.8;
    quint16 buttonHeight = mItems->boundingRect().height();
    buttonHeight *= mItems->getSettings().infoHeightPortion;
    realTimeButton->setFixedSize(buttonHeight, buttonHeight);
    realTimeButton->setIconSize(QSize(realTimeButton->width()*sizeMult, realTimeButton->height()*sizeMult));
    mRealTimeButtonProxy->setGeometry(QRect(graphicsRect.left(), graphicsRect.top(), realTimeButton->width(), realTimeButton->height()));

    scene()->setSceneRect(mGrid->boundingRect());
}

void TimeLineWidget::setRealTime()
{
    QPushButton* realTimeButton = (QPushButton*)mRealTimeButtonProxy->widget();
    if (realTimeButton->isChecked())
    {
        realTimeButton->setIcon(QIcon(":/Sphere/Resources/sphere_timeline_clock_32.png"));
        bool ok = mGrid->setTimeRange(QDateTime::currentDateTime(), mGrid->getTimeDelta());

        if (ok)
        {
            mItems->setTime(mGrid->getTimeMark(), mGrid->getTimeDelta());
        }
    }
    else{
        realTimeButton->setIcon(QIcon(":/Sphere/Resources/sphere_timeline_clock_offline_32.png"));
    }
}

void TimeLineWidget::setStyle(const TimeLineStyle& style)
{
    mItems->setStyle(style.itemsStyle);
    mGrid->setStyle(style.gridStyle);
}

void TimeLineWidget::setSettings(const TimeLineSettings& settings)
{
    mItems->setSettings(settings.itemsSettings);
    mGrid->setSettings(settings.gridSettings);
}

TimeLineWidget::TimeLineStyle TimeLineWidget::getStyle() const
{
    TimeLineStyle style;
    style.gridStyle = mGrid->getStyle();
    style.itemsStyle = mItems->getStyle();

    return style;
}

TimeLineWidget::TimeLineSettings TimeLineWidget::getSettings() const
{
    TimeLineSettings settings;
    settings.gridSettings = mGrid->getSettings();
    settings.itemsSettings = mItems->getSettings();

    return settings;
}

