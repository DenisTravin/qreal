#include "editorviewscene.h"
#include "math.h"

#include <QGraphicsTextItem>
#include <QtGui>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QGraphicsItem>

#include "editorviewmviface.h"
#include "editorview.h"
#include "mainwindow.h"
#include "../mainwindow/mainwindow.h"
#include "../model/model.h"

using namespace qReal;

EditorViewScene::EditorViewScene(QObject * parent)
	:  QGraphicsScene(parent), mWindow(NULL), mPrevParent(0)
{
	QSettings settings("SPbSU", "QReal");
	mNeedDrawGrid = settings.value("ShowGrid", true).toBool();
	mWidthOfGrid = static_cast<double>(settings.value("GridWidth", 10).toInt()) / 100;
	setItemIndexMethod(NoIndex);
	setEnabled(false);
	mRightButtonPressed = false;

	mActionSignalMapper = new QSignalMapper(this);
}

EditorViewScene::~EditorViewScene()
{
	delete mActionSignalMapper;
}

void EditorViewScene::drawIdealGesture()
{
	mouseMovementManager->drawIdealPath();
}

void EditorViewScene::printElementsOfRootDiagram()
{
	mouseMovementManager->setGesturesPainter(mWindow->gesturesPainter());
	mouseMovementManager->printElements();
}

void EditorViewScene::initMouseMoveManager()
{
	if (!model())
		return;
        qReal::NewType diagram = model()->getRootDiagram();
        if (diagram == NewType())
		// Root diagram is not set, for example, current tab is disabled. No need
		// to do anything with mouse manager.
		return;
        QList<qReal::NewType> elements = mWindow->manager()->elementsOnDiagram(diagram);
	mouseMovementManager = new MouseMovementManager(elements,
			mWindow->manager(), mWindow->gesturesPainter());
	connect(mWindow, SIGNAL(currentIdealGestureChanged()), this, SLOT(drawIdealGesture()));
	connect(mWindow, SIGNAL(gesturesShowed()), this, SLOT(printElementsOfRootDiagram()));
}

void EditorViewScene::drawGrid(QPainter *painter, const QRectF &rect)
{
	qreal sceneX = rect.x(), sceneY = rect.y();
	int startX = static_cast<int>(sceneX + 10) / indexGrid * indexGrid;
	int endX = static_cast<int>(sceneX + rect.width() - 10) / indexGrid * indexGrid;
	int startY = static_cast<int>(sceneY + 10) / indexGrid * indexGrid;
	int endY = static_cast<int>(sceneY + rect.height() - 10) / indexGrid * indexGrid;
	for (int i = startX; i <= endX; i = i + indexGrid) {
		QLineF line(i, startY, i, endY);
		painter->drawLine(line);
	}
	for (int i = startY; i <= endY; i = i + indexGrid) {
		QLineF line(startX, i, endX, i);
		painter->drawLine(line);
	}
}

void EditorViewScene::setEnabled(bool enabled)
{
	foreach (QGraphicsView *view, views())
		view->setEnabled(enabled);
}

void EditorViewScene::clearScene()
{
	foreach (QGraphicsItem *item, items())
		// looks really insane, but some elements were alreadt deleted together with their parent
		if (items().contains(item))
			removeItem(item);
}

UML::Element * EditorViewScene::getElem(qReal::Id const &id)
{
        if (id == ROOT_ID)
		return NULL;

	// FIXME: SLOW!
	QList < QGraphicsItem * > list = items();
	for (QList < QGraphicsItem * >::Iterator it = list.begin(); it != list.end(); ++it) {
		if (UML::Element * elem = dynamic_cast < UML::Element * >(*it)) {
                        if (elem->id() == id) {
				return elem;
			}
		}
	}

	return NULL;
}

UML::Element * EditorViewScene::getElemByModelIndex(const QModelIndex &ind)
{
	// FIXME: SLOW!
	QList < QGraphicsItem * > list = items();
	for (QList < QGraphicsItem * >::Iterator it = list.begin(); it != list.end(); ++it) {
		if (UML::Element * elem = dynamic_cast < UML::Element * >(*it)) {
			if (elem->index() == ind)
				return elem;
		}
	}
	return NULL;
}

void EditorViewScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
	const QMimeData *mimeData = event->mimeData();
	if (mimeData->hasFormat("application/x-real-uml-data"))
		QGraphicsScene::dragEnterEvent(event);
	else
		event->ignore();
}

void EditorViewScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
	Q_UNUSED(event);
}

void EditorViewScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
	Q_UNUSED(event);
}

bool EditorViewScene::canBeContainedBy(qReal::NewType container, qReal::NewType candidate)
{
	Q_UNUSED(container);
	Q_UNUSED(candidate); // TODO: update xml descriptions to remove
	return true;

	/*
	bool allowed = false;
	foreach (qReal::Id type, mWindow->manager()->getContainedTypes(container.type())){
		if (candidate.element() ==  type.editor())
			allowed = true;
	}
	return allowed;
	*/
}

void EditorViewScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
	Q_ASSERT(mWindow); // should be initialized separately.
	// constuctor is bad for this, because the scene is created in generated .ui file

	// if there's no diagrams. create nothing
	if (mv_iface->model()->rowCount(QModelIndex()) == 0)
		return;

	createElement(event->mimeData(),event->scenePos());
}

int EditorViewScene::launchEdgeMenu(UML::EdgeElement* edge, UML::NodeElement* node, QPointF scenePos)
{
	edge->setSelected(true);

	QList<QObject*> toDelete;

	QMenu *edgeMenu = new QMenu();
	toDelete.append(edgeMenu);
	edgeMenu->addAction(mWindow->ui.actionDeleteFromDiagram);
	edgeMenu->addAction(QString("Discard"));
	edgeMenu->addSeparator();

	QMenu *createElemMenu = new QMenu(QString("Create new element"), edgeMenu);
	toDelete.append(createElemMenu);
	edgeMenu->addMenu(createElemMenu);

	QSignalMapper *menuSignalMapper = new QSignalMapper(this);
	toDelete.append(menuSignalMapper);

	foreach(UML::PossibleEdge pEdge, edge->getPossibleEdges())
	{
		QString target;
                if (pEdge.first.first.element() == node->newType().element())
			target = pEdge.first.second.element();
                else if ((pEdge.first.second.element() == node->newType().element()) && (!pEdge.second.first))
			target = pEdge.first.first.element();
		else continue;

		QAction* element = new QAction(target, createElemMenu);
		createElemMenu->addAction(element);
		toDelete.append(element);
		QObject::connect(element,SIGNAL(triggered()), menuSignalMapper,SLOT(map()));
                menuSignalMapper->setMapping(element, "qrm:/"+node->newType().editor()+"/"+node->newType().diagram()+"/"+target);
	}

	mCreatePoint = scenePos;
	QObject::connect(menuSignalMapper,SIGNAL(mapped(const QString &)),this,SLOT(createElement(const QString &)));

	QAction* executed;
	QPoint cursorPos = QCursor::pos();
	executed = edgeMenu->exec(cursorPos);

	int result = 0;
	if (executed) {
		if (executed == mWindow->ui.actionDeleteFromDiagram)
			result = -1;
		else if (!(executed->text() == "Discard"))
			result = +1;
	}

	//	Cleaning.
	//	foreach(QObject *object, toDelete)
	//		delete object;

	return result;
}

qReal::Id *EditorViewScene::createElement(const QString &str)
{
        qReal::Id* result = createElement(str, mCreatePoint);
	lastCreatedWithEdge = getElem(*result);
	return result;
}

qReal::Id *EditorViewScene::createElement(const QString &str, QPointF scenePos)
{
        NewType typeId = NewType::loadFromString(str);
        NewType *objectType = new NewType(typeId.editor(),typeId.diagram(),typeId.element());
        Id *objectId = new Id(QUuid::createUuid().toString());

	QByteArray data;
	QMimeData *mimeData = new QMimeData();
	QDataStream stream(&data, QIODevice::WriteOnly);
	QString mimeType = QString("application/x-real-uml-data");
	QString uuid = objectId->toString();
        QString type = objectType->toString();
	QString pathToItem = ROOT_ID.toString();
	QString name = "(anonymous something)";
	QPointF pos = QPointF(0, 0);
	stream << uuid;
        stream << type;
	stream << pathToItem;
	stream << name;
	stream << pos;

	mimeData->setData(mimeType, data);
	createElement(mimeData, scenePos);
	delete mimeData;

        return objectId;
}

void EditorViewScene::createElement(const QMimeData *mimeData, QPointF scenePos)
{
	QByteArray itemData = mimeData->data("application/x-real-uml-data");
	QDataStream in_stream(&itemData, QIODevice::ReadOnly);

	QString uuid = "";
        QString elemtype = "";
	QString pathToItem = "";
	QString name;
	QPointF pos;
	in_stream >> uuid;
        in_stream >> elemtype;
	in_stream >> pathToItem;
	in_stream >> name;
	in_stream >> pos;

	QByteArray newItemData;
	QDataStream stream(&newItemData, QIODevice::WriteOnly);

	UML::Element *newParent = NULL;

	// TODO: make it simpler
        qReal::NewType type = qReal::NewType::loadFromString(elemtype);
        UML::Element* e = mWindow->manager()->graphicalObject(type);

	// TODO: what is it??
	if (dynamic_cast<UML::NodeElement*>(e))
		newParent = getElemAt(scenePos);

	if (e)
		delete e;

	if( newParent ){
                if (!canBeContainedBy(newParent->newType(), type)){
			QMessageBox::critical(0, "Error!", "[some text]");
			return;
		}

		//temporary solution for chaotic changes of coordinates of created elements with edge menu
		UML::EdgeElement* edge = dynamic_cast<UML::EdgeElement*>(newParent);
		if (edge)
			newParent = NULL;
	}

	stream << uuid;
        stream << elemtype;
	stream << pathToItem;
	stream << name;

	if (!newParent)
		stream << scenePos;
	else
		stream << newParent->mapToItem(newParent, newParent->mapFromScene(scenePos));

	QMimeData *newMimeData = new QMimeData;
	newMimeData->setData("application/x-real-uml-data", newItemData);
	QModelIndex parentIndex = newParent ? QModelIndex(newParent->index()) : mv_iface->rootIndex();
	mv_iface->model()->dropMimeData(newMimeData, Qt::CopyAction,
									mv_iface->model()->rowCount(parentIndex), 0, parentIndex);

        emit elementCreated(type);

	delete newMimeData;
}

UML::Element* EditorViewScene::getLastCreated()
{
	return lastCreatedWithEdge;
}

void EditorViewScene::keyPressEvent(QKeyEvent *event)
{
	if (dynamic_cast<QGraphicsTextItem*>(focusItem())) {
		// Forward event to text editor
		QGraphicsScene::keyPressEvent(event);
	} else if (event->key() == Qt::Key_Delete) {
		// Delete selected elements from scene
		mainWindow()->deleteFromScene();
	} else
		QGraphicsScene::keyPressEvent(event);
}

void EditorViewScene::createGoToSubmenu(QMenu * const goToMenu, QString const &name, qReal::IdList const &ids) const
{
	QMenu *menu = goToMenu->addMenu(name);
        foreach (Id element, ids) {
		QAction *action = menu->addAction(model()->api().name(element));
		connect(action, SIGNAL(triggered()), SLOT(goToActionTriggered()));
		action->setData(element.toVariant());
	}
}

void EditorViewScene::createAddConnectionMenu(UML::Element const * const element
											  , QMenu &contextMenu, QString const &menuName
                                                                                          , TypeList const &connectableIds, IdList const &alreadyConnectedElements
                                                                                          , TypeList const &connectableDiagrams, const char *slot) const
{
	QMenu *addConnectionMenu = contextMenu.addMenu(menuName);

        foreach (NewType id, connectableIds) {
                foreach (Id elementId, model()->api().elementsByType(id.element())) {
                        if (alreadyConnectedElements.contains(elementId))
				continue;
                        QAction *action = addConnectionMenu->addAction(model()->api().name(elementId));
			connect(action, SIGNAL(triggered()), slot);
			QList<QVariant> tag;
                        //tag << element->newType().toVariant() << elementId.toVariant();
			action->setData(tag);
		}
	}

        foreach (NewType diagram, connectableDiagrams) {
                NewType diagramType = model()->assistApi().editorManager().findElementByType(diagram.element());
                QString name = model()->assistApi().editorManager().friendlyName(diagramType);
                QString editorName = model()->assistApi().editorManager().friendlyName(diagram.editorId());
		QAction *action = addConnectionMenu->addAction("New " + editorName + "/" + name);
		connect(action, SIGNAL(triggered()), slot);
		QList<QVariant> tag;
                tag << element->newType().toVariant() << diagramType.toVariant();
		action->setData(tag);
	}
}

void EditorViewScene::createDisconnectMenu(UML::Element const * const element
										   , QMenu &contextMenu, QString const &menuName
                                                                                   , IdList const &outgoingConnections, IdList const &incomingConnections
										   , const char *slot) const
{
	QMenu *disconnectMenu = contextMenu.addMenu(menuName);
        IdList list = outgoingConnections;
	list.append(incomingConnections);

        foreach (Id elementId, list) {
                QAction *action = disconnectMenu->addAction(model()->api().name(elementId));
		connect(action, SIGNAL(triggered()), slot);
		QList<QVariant> tag;
                //tag << element->type().toVariant() << elementId.toVariant();
		action->setData(tag);
	}
}

void EditorViewScene::createConnectionSubmenus(QMenu &contextMenu, UML::Element const * const element) const
{
	// menu items "connect to"
	// TODO: move to elements, they can call the model and API themselves
	createAddConnectionMenu(element, contextMenu, tr("Add connection")
                                                        , mWindow->manager()->getConnectedTypes(element->newType())
                                                        , model()->api().outgoingConnections(element->id())
                                                        , model()->assistApi().diagramsAbleToBeConnectedTo(element->newType())
							, SLOT(connectActionTriggered())
							);

	createDisconnectMenu(element, contextMenu, tr("Disconnect")
                                                 , model()->api().outgoingConnections(element->id())
                                                 , model()->api().incomingConnections(element->id())
						 , SLOT(disconnectActionTriggered())
						 );

	createAddConnectionMenu(element, contextMenu, tr("Add usage")
                                                        , mWindow->manager()->getUsedTypes(element->newType())
                                                        , model()->api().outgoingUsages(element->id())
                                                        , model()->assistApi().diagramsAbleToBeUsedIn(element->newType())
							, SLOT(addUsageActionTriggered())
							);

	createDisconnectMenu(element, contextMenu, tr("Delete usage")
                                                 , model()->api().outgoingUsages(element->id())
                                                 , model()->api().incomingUsages(element->id())
						 , SLOT(deleteUsageActionTriggered())
						 );

	QMenu *goToMenu = contextMenu.addMenu(tr("Go to"));

        createGoToSubmenu(goToMenu, tr("Forward connection"), model()->api().outgoingConnections(element->id()));
        createGoToSubmenu(goToMenu, tr("Backward connection"), model()->api().incomingConnections(element->id()));
        createGoToSubmenu(goToMenu, tr("Uses"), model()->api().outgoingUsages(element->id()));
        createGoToSubmenu(goToMenu, tr("Used in"), model()->api().incomingUsages(element->id()));
}

void EditorViewScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	// Let scene update selection and perform other operations
	QGraphicsScene::mousePressEvent(event);

	if( event->button() == Qt::LeftButton ){
		QGraphicsItem *item = itemAt(event->scenePos());
		UML::ElementTitle *title = dynamic_cast < UML::ElementTitle * >(item);

		if (title) // check whether we accidently clicked on a title or not
			item = item->parentItem();

		if (item) {
			mPrevParent = item->parentItem();
			mPrevPosition = item->pos();
		}

	} else if (event->button() == Qt::RightButton) {
		UML::Element *e = getElemAt(event->scenePos());
		//	if (!e) {
		mouseMovementManager->addPoint(event->scenePos());
		mRightButtonPressed = true;
		//		return;
		//	}
		if (e && !e->isSelected()) {
			clearSelection();
			e->setSelected(true);
		}

		// Menu belongs to scene handler because it can delete elements.
		// We cannot allow elements to commit suicide.
		//if (e)
		//	initContextMenu(e, event->scenePos());
	}
	if (mNeedDrawGrid)
		invalidate();
}

void EditorViewScene::initContextMenu(UML::Element *e, const QPointF &pos)
{
	QMenu menu;
	menu.addAction(mWindow->ui.actionDeleteFromDiagram);
	QList<UML::ContextMenuAction*> elementActions = e->contextMenuActions();

	if (!elementActions.isEmpty())
		menu.addSeparator();

	foreach (UML::ContextMenuAction* action, elementActions) {
		action->setEventPos(e->mapFromScene(pos));
		menu.addAction(action);

		connect(action, SIGNAL(triggered()), mActionSignalMapper, SLOT(map()),
				Qt::UniqueConnection);
                mActionSignalMapper->setMapping(action, action->text() + "###" + e->newType().toString());
	}
	menu.addSeparator();

	createConnectionSubmenus(menu, e);

	menu.exec(QCursor::pos());
}

void EditorViewScene::getObjectByGesture()
{
	deleteGesture();
	QPointF start = mouseMovementManager->firstPoint();
	QPointF end = mouseMovementManager->lastPoint();
	UML::NodeElement * parent = dynamic_cast <UML::NodeElement * > (getElemAt(start));
	UML::NodeElement * child = dynamic_cast <UML::NodeElement * > (getElemAt(end));
	if (parent && child)
	{
		getLinkByGesture(parent, *child);
	}
	else
	{
                qReal::NewType type = mouseMovementManager->getObject();
                if (type.element() != "")
                        createElement(type.toString(), mouseMovementManager->pos());
	}
}

void EditorViewScene::getLinkByGesture(UML::NodeElement * parent, const UML::NodeElement &child)
{
	QList<UML::PossibleEdge> edges = parent->getPossibleEdges();
	QList<QString> allLinks;
	foreach (UML::PossibleEdge possibleEdge, edges)
	{
                if (possibleEdge.first.second.editor() == child.newType().editor()
                        && possibleEdge.first.second.diagram() == child.newType().diagram()
                        && possibleEdge.first.second.element() == child.newType().element())
			allLinks.push_back(possibleEdge.second.second.toString());
	}
	if (!allLinks.empty())
	{
		if (allLinks.count() == 1)
			createEdge(allLinks.at(0));
		else
			createEdgeMenu(allLinks);
	}
}

void EditorViewScene::createEdgeMenu(const QList<QString> &ids)
{
	QMenu * edgeMenu = new QMenu();
	QSignalMapper * menuSignalMapper = new QSignalMapper(this);
	foreach (QString id, ids)
	{
		QAction * element = new QAction(id, edgeMenu);
		edgeMenu->addAction(element);
		QObject::connect(element, SIGNAL(triggered()), menuSignalMapper, SLOT(map()));
		menuSignalMapper->setMapping(element, id);
	}
	QObject::connect(menuSignalMapper, SIGNAL(mapped(const QString &)), this,
					 SLOT(createEdge(QString)));
	edgeMenu->exec(QCursor::pos());
}

void EditorViewScene::createEdge(const QString & idStr)
{
	QPointF start = mouseMovementManager->firstPoint();
	QPointF end = mouseMovementManager->lastPoint();
	UML::NodeElement * child = dynamic_cast <UML::NodeElement * > (getElemAt(end));
        Id * id = createElement(idStr, start);
        UML::Element * edgeElement = getElem(*id);
	UML::EdgeElement * edge = dynamic_cast <UML::EdgeElement * > (edgeElement);
	QPointF endPos = edge->mapFromItem(child, child->getNearestPort(end));
	edge->placeEndTo(endPos);
	edge->connectToPort();
}

void EditorViewScene::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event )
{
	QGraphicsScene::mouseReleaseEvent(event);

	UML::Element* element = getElemAt(event->scenePos());

	if (event->button() == Qt::RightButton)
	{
		mouseMovementManager->addPoint(event->scenePos());
		if (mouseMovementManager->wasMoving())
			getObjectByGesture();
		else if (element)
			initContextMenu(element, event->scenePos());
		mRightButtonPressed = false;
		mouseMovementManager->clear();
		return;
	}

	if (!element)
		return;
	UML::Element *parent;
	parent = getElemByModelIndex(element->index().parent());
	if (parent) {
                if (!canBeContainedBy(parent->newType(), element->newType())){
			QMessageBox::critical(0, "Ololo", "can't drop it here!111");
			// fail, reparenting the element as it was before
			foreach (QGraphicsItem *item, items(event->scenePos())) {
				UML::Element * elem = dynamic_cast < UML::Element * >(item);
                                if (elem && elem->newType() == element->newType()) {
					QModelIndex ind = mv_iface->rootIndex();
					UML::Element * prevParent = dynamic_cast < UML::Element * >(mPrevParent);
					if (prevParent)
						ind = prevParent->index();
					if (model())
						model()->changeParent(element->index(), ind, mPrevPosition);
				}
			}
		}
	}
	if (mNeedDrawGrid)
		invalidate();
}

void EditorViewScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	// button isn't recognized while mouse moves
	if (mRightButtonPressed)
	{
		mouseMovementManager->addPoint(event->scenePos());
		drawGesture();
	}
	else
		QGraphicsScene::mouseMoveEvent(event);
}


void EditorViewScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		// Double click on a title activates it
		if (UML::ElementTitle *title = dynamic_cast<UML::ElementTitle*>(itemAt(event->scenePos()))) {
			if (!title->hasFocus()) {  // Do not activate already activated item
				event->accept();
				title->startTextInteraction();
				return;
			}
		}
		else if (UML::NodeElement *element = dynamic_cast<UML::NodeElement*>(itemAt(event->scenePos()))) {
			event->accept();

                        IdList outgoingLinks = model()->api().outgoingConnections(element->id());

			if (outgoingLinks.size() > 0)
				mainWindow()->activateItemOrDiagram(outgoingLinks[0]);
			else {
                                TypeList diagrams = model()->assistApi().diagramsAbleToBeConnectedTo(element->newType());
				if (!diagrams.isEmpty()) {
                                        NewType diagramType = model()->assistApi().editorManager().findElementByType(diagrams[0].element());
                                        model()->assistApi().createConnected(element->id(), diagramType);
				}
			}

			// Now scene is changed from outside. Being a mere mortal I do not
			// know whether it is good or not, but what is the destiny of
			// workflow after this return?
			return;
		}
	}

	QGraphicsScene::mouseDoubleClickEvent(event);
}

UML::Element* EditorViewScene::getElemAt(QPointF const &position)
{
	foreach (QGraphicsItem *item, items(position)) {
		UML::Element* e = dynamic_cast<UML::Element*>(item);
		if (e)
			return e;
	}
	return NULL;
}

QPersistentModelIndex EditorViewScene::rootItem()
{
	return mv_iface->rootIndex();
}

void EditorViewScene::setMainWindow(qReal::MainWindow *mainWindow)
{
	mWindow = mainWindow;
	connect(mWindow, SIGNAL(rootDiagramChanged()), this, SLOT(initMouseMoveManager()));
        connect(this, SIGNAL(elementCreated(qReal::NewType)), mainWindow->listenerManager(), SIGNAL(objectCreated(qReal::NewType)));
	connect(mActionSignalMapper, SIGNAL(mapped(QString)), mainWindow->listenerManager(), SIGNAL(contextMenuActionTriggered(QString)));
}

qReal::MainWindow *EditorViewScene::mainWindow() const
{
	return mWindow;
}

void EditorViewScene::connectActionTriggered()
{
	QAction *action = static_cast<QAction *>(sender());
	QList<QVariant> connection = action->data().toList();
        Id source = connection[0].value<Id>();
        Id destination = connection[1].value<Id>();
	if (!action->text().startsWith("New ")) {
		model()->assistApi().connect(source, destination);
	} else {
                model()->assistApi().createConnected(source, model()->api().type(destination));
	}
}

void EditorViewScene::addUsageActionTriggered()
{
	QAction *action = static_cast<QAction *>(sender());
	QList<QVariant> connection = action->data().toList();
        Id source = connection[0].value<Id>();
        Id destination = connection[1].value<Id>();
	if (!action->text().startsWith("New ")) {
		model()->assistApi().addUsage(source, destination);
	} else {
                model()->assistApi().createUsed(source, model()->api().type(destination));
	}
}

void EditorViewScene::goToActionTriggered()
{
	QAction *action = static_cast<QAction *>(sender());
        Id target = action->data().value<Id>();
	mainWindow()->activateItemOrDiagram(target);
	return;
}

void EditorViewScene::disconnectActionTriggered()
{
	QAction *action = static_cast<QAction *>(sender());
	QList<QVariant> connection = action->data().toList();
        Id source = connection[0].value<Id>();
        Id destination = connection[1].value<Id>();
	model()->assistApi().disconnect(source, destination);
}

void EditorViewScene::deleteUsageActionTriggered()
{
	QAction *action = static_cast<QAction *>(sender());
	QList<QVariant> connection = action->data().toList();
        Id source = connection[0].value<Id>();
        Id destination = connection[1].value<Id>();
	model()->assistApi().deleteUsage(source, destination);
}

qReal::model::Model *EditorViewScene::model() const
{
	return dynamic_cast<qReal::model::Model *>(mv_iface->model());
}

void EditorViewScene::drawBackground(QPainter *painter, const QRectF &rect)
{
	if (mNeedDrawGrid) {
		QSettings settings("SPbSU", "QReal");
		mWidthOfGrid = (settings.value("GridWidth", 10).toDouble()) / 100;
		painter->setPen(QPen(Qt::black, mWidthOfGrid));
		drawGrid(painter, rect);
	}
}

void EditorViewScene::setNeedDrawGrid(bool show)
{
	mNeedDrawGrid = show;
}

bool EditorViewScene::getNeedDrawGrid()
{
	return mNeedDrawGrid;
}

void EditorViewScene::drawGesture()
{
	QLineF line = mouseMovementManager->newLine();
	QGraphicsLineItem * item = new QGraphicsLineItem(line, NULL, this);
	double size = mGesture.size() * 0.1;
	double color_ratio = pow(fabs(sin(size)), 1.5);
	QColor penColor(255 * color_ratio, 255 * (1 - color_ratio), 255);
	item->setPen(penColor);
	addItem(item);
	mGesture.push_back(item);
	int ellipseSize = 2;
	QPointF bound(ellipseSize, ellipseSize);
	QRectF rect(line.p2() + bound, line.p2() - bound);
	QGraphicsEllipseItem * ellipseItem = new QGraphicsEllipseItem(rect, NULL, this);
	ellipseItem->setPen(penColor);
	addItem(ellipseItem);
	mGesture.push_back(ellipseItem);
}

void EditorViewScene::deleteGesture()
{
	foreach (QGraphicsItem * item, mGesture)
	{
		removeItem(item);
	}
	mGesture.clear();
}

void EditorViewScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
	if (wheelEvent->delta() > 0)
		emit zoomIn();
	else
		emit zoomOut();
	wheelEvent->accept();
	return;
}