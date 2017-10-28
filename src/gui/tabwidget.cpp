// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/tabwidget.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/plaintoolbutton.h"
#include "gui/tabbar.h"
#include "gui/texteditor.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"

#include <QMenu>
#include <QToolButton>

TabWidget::TabWidget(QWidget* parent) : QTabWidget(parent), m_menuMain(nullptr) {
  setTabBar(new TabBar(this));
  setupMainMenuButton();
  initializeTabs();
  createConnections();
}

TabWidget::~TabWidget() {
  qDebug("Destroying TabWidget instance.");
}

void TabWidget::setupMainMenuButton() {
  m_btnMainMenu = new PlainToolButton(this);
  m_btnMainMenu->setAutoRaise(true);
  m_btnMainMenu->setPadding(3);
  m_btnMainMenu->setToolTip(tr("Displays main menu."));
  m_btnMainMenu->setIcon(qApp->icons()->fromTheme(QSL("go-home")));
  m_btnMainMenu->setPopupMode(QToolButton::InstantPopup);

  connect(m_btnMainMenu, &PlainToolButton::clicked, this, &TabWidget::openMainMenu);
}

void TabWidget::openMainMenu() {
  if (m_menuMain == nullptr) {
    m_menuMain = new QMenu(tr("Main menu"), this);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuFile);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuView);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuWebBrowserTabs);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuTools);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuHelp);
  }

  QPoint button_position = m_btnMainMenu->pos();
  const QSize target_size = m_btnMainMenu->size() / 2.0;

  button_position.setX(button_position.x() + target_size.width());
  button_position.setY(button_position.y() + target_size.height());
  m_menuMain->exec(mapToGlobal(button_position));
}

void TabWidget::showDownloadManager() {
  for (int i = 0; i < count(); i++) {
    if (widget(i)->metaObject()->className() == QSL("DownloadManager")) {
      setCurrentIndex(i);
      return;
    }
  }

  // Download manager is not opened. Create tab with it.
  qApp->downloadManager()->setParent(this);
  addTab(qApp->downloadManager(), qApp->icons()->fromTheme(QSL("emblem-downloads")), tr("Downloads"), TabBar::DownloadManager);
  setCurrentIndex(count() - 1);
}

void TabWidget::checkTabBarVisibility() {
  const bool should_be_visible = count() > 1 || !qApp->settings()->value(GROUP(GUI), SETTING(GUI::HideTabBarIfOnlyOneTab)).toBool();

  if (should_be_visible) {
    setCornerWidget(m_btnMainMenu, Qt::TopLeftCorner);
    m_btnMainMenu->setVisible(true);
  }
  else {
    setCornerWidget(0, Qt::TopLeftCorner);
    setCornerWidget(0, Qt::TopRightCorner);
    m_btnMainMenu->setVisible(false);
  }

  tabBar()->setVisible(should_be_visible);
}

void TabWidget::tabInserted(int index) {
  QTabWidget::tabInserted(index);
  checkTabBarVisibility();
  const int count_of_tabs = count();

  if (index < count_of_tabs - 1 && count_of_tabs > 1) {
    // New tab was inserted and the tab is not the last one.
    fixContentsAfterMove(index, count_of_tabs - 1);
  }
}

void TabWidget::tabRemoved(int index) {
  QTabWidget::tabRemoved(index);
  checkTabBarVisibility();
  const int count_of_tabs = count();

  if (index < count_of_tabs && count_of_tabs > 1) {
    // Some tab was removed and the tab was not the last one.
    fixContentsAfterMove(index, count_of_tabs - 1);
  }
}

void TabWidget::createConnections() {
  connect(tabBar(), &TabBar::tabCloseRequested, this, &TabWidget::closeTab);
  connect(tabBar(), &TabBar::emptySpaceDoubleClicked, this, &TabWidget::addNewEmptyTab);
  connect(tabBar(), &TabBar::tabMoved, this, &TabWidget::fixContentsAfterMove);
}

void TabWidget::initializeTabs() {
  // TODO: tady vytvořit tabovou instanci text editoru
  QTabWidget::addTab(new TextEditor(), "text");
}

void TabWidget::setupIcons() {
  // Iterate through all tabs and update icons
  // accordingly.
  for (int index = 0; index < count(); index++) {
    // Index 0 usually contains widget which displays feeds & messages.
    if (tabBar()->tabType(index) == TabBar::TextEditor) {
      setTabIcon(index, qApp->icons()->fromTheme(QSL("application-text")));
    }
  }
}

bool TabWidget::closeTab(int index) {
  if (tabBar()->tabType(index) == TabBar::Closable) {
    removeTab(index, true);
    return true;
  }
  else if (tabBar()->tabType(index) == TabBar::DownloadManager) {
    removeTab(index, false);
    return true;
  }
  else {
    return false;
  }
}

void TabWidget::closeAllTabsExceptCurrent() {
  // Close tabs after active tab.
  int index_of_active = currentIndex();

  for (int i = count() - 1; i >= 0; i--) {
    if (i != index_of_active) {
      if (i < index_of_active) {
        index_of_active--;
      }

      closeTab(i);
    }
  }
}

void TabWidget::closeAllTabs() {
  for (int i = count() - 1; i >= 0; i--) {
    closeTab(i);
  }
}

void TabWidget::addNewEmptyTab() {
  // TODO: pridat novy prazdny dokument
}

void TabWidget::gotoNextTab() {
  if (currentIndex() == count() - 1) {
    setCurrentIndex(0);
  }
  else {
    setCurrentIndex(currentIndex() + 1);
  }
}

void TabWidget::gotoPreviousTab() {
  if (currentIndex() == 0) {
    setCurrentIndex(count() - 1);
  }
  else {
    setCurrentIndex(currentIndex() - 1);
  }
}

void TabWidget::indentTabText(int index) {
#if defined (Q_OS_MACOS)
  if (tabBar()->tabType(index) != TabBar::FeedReader && !tabIcon(index).isNull()) {
    // We have closable tab with some icon, fix the title.
    const QString text = tabText(index);

    if (!text.startsWith(QSL("  "))) {
      setTabText(index, QSL("  ") + text);
    }
  }
#else
  Q_UNUSED(index)
#endif
}

void TabWidget::removeTab(int index, bool clear_from_memory) {
  if (clear_from_memory) {
    widget(index)->deleteLater();
  }

  QTabWidget::removeTab(index);
}

int TabWidget::addTab(TabContent* widget, const QIcon& icon, const QString& label, const TabBar::TabType& type) {
  const int index = QTabWidget::addTab(widget, icon, label);

  tabBar()->setTabType(index, type);
  indentTabText(index);
  return index;
}

int TabWidget::addTab(TabContent* widget, const QString& label, const TabBar::TabType& type) {
  const int index = QTabWidget::addTab(widget, label);

  tabBar()->setTabType(index, type);
  indentTabText(index);
  return index;
}

int TabWidget::insertTab(int index, QWidget* widget, const QIcon& icon, const QString& label, const TabBar::TabType& type) {
  const int tab_index = QTabWidget::insertTab(index, widget, icon, label);

  tabBar()->setTabType(tab_index, type);
  indentTabText(index);
  return tab_index;
}

int TabWidget::insertTab(int index, QWidget* widget, const QString& label, const TabBar::TabType& type) {
  const int tab_index = QTabWidget::insertTab(index, widget, label);

  tabBar()->setTabType(tab_index, type);
  indentTabText(index);
  return tab_index;
}

void TabWidget::changeIcon(int index, const QIcon& new_icon) {
  setTabIcon(index, new_icon);
  indentTabText(index);
}

void TabWidget::changeTitle(int index, const QString& new_title) {
  setTabText(index, new_title);
  setTabToolTip(index, new_title);
  indentTabText(index);
}

void TabWidget::fixContentsAfterMove(int from, int to) {
  from = qMin(from, to);
  to = qMax(from, to);

  for (; from <= to; from++) {
    TabContent* content = static_cast<TabContent*>(widget(from));

    content->setIndex(from);
  }
}