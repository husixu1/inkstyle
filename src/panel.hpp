#ifndef PANEL_H
#define PANEL_H

#include "button.hpp"
#include "configmanager.hpp"
#include "hiddenbutton.hpp"

#include <QPushButton>
#include <QSharedPointer>
#include <QStack>
#include <QVector>
#include <QWeakPointer>
#include <QWidget>

/// @brief A Panel is a hexagon that contains multiple buttons.
class Panel : public QWidget {
    Q_OBJECT
public:
    Panel(Panel *parent = nullptr, quint8 tSlot = 0);
    virtual ~Panel() override;

    /// @brief Add buttons that applies style to inkscape objects
    /// @param tSlot Theta(angle)-slot, 0~5
    /// @param rSlot Radius-slot, 1~2
    /// @param subSlot Sub-slot when angle and radius is defined,
    /// 0~(rSlot*2)
    /// @details
    /// `````````````````````````````````````````
    /// |           \    tSlot=1    /           |
    /// |                                       |
    /// |             •---•---•---•             |
    /// |            / \4/3\2/1\0/4\            |
    /// | tSlot=2   •---•---•---•---•   tSlot=0 |
    /// |          / \ / \2/1\0/2\3/2\          |
    /// |         •---•---•---•---•---•         |
    /// |        / \ / \ /     \1/0\1/0\        |
    /// |  ---  •-b-•-a-•   C   •-a-•-b-•  ---  |
    /// |                                       |
    /// | C: center                             |
    /// | a: rSlot=1, b: rSlot=2                |
    /// | Numbers in cells are subSlots         |
    /// `````````````````````````````````````````
    Button *addStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief Add button that fills border of the hexagon
    /// @param tSlot theta(angle)-slot, range from 0-5
    HiddenButton *addBorderButton(quint8 tSlot);
    void delBorderButton(quint8 tSlot);

    static void setConfig(ConfigManager *newConfig);

    static QPixmap drawIcon(
        const QSizeF &iconSize, const QPointF &centroid,
        const ConfigManager::ButtonInfo &button);

protected:
    /// @brief Overridden to recursively move all panels
    void moveEvent(QMoveEvent *event) override;
    /// @brief Overridden to recursively close all panels
    void closeEvent(QCloseEvent *event) override;
    /// @brief Overridden to update visual guides
    void paintEvent(QPaintEvent *event) override;
    /// @brief Overridden to close all children
    void enterEvent(QEvent *event) override;

private:
    /// @brief Update buttons at the border of the panel
    void updateBorderButtons();

    /// @brief Update masked area
    void updateMask();

    /// @brief Calculate absolute position of target panel (relative to this)
    /// @param[in] tSlot The target panel occupies tSlot of this panel
    /// @return Coordinate of the target panel
    /// @see #panelGrid
    inline QPoint calcRelativeCoordinate(quint8 tSlot);

    /// @brief Calculate absolute position of target panel
    /// @param[in] tSlot The target panel occupies tSlot of this panel
    /// @return Absolute position of the target panel
    inline QPoint calcRelativePanelPos(quint8 tSlot);

    /// @brief Generate mask for a border-button
    /// @param tSlot The theta-slot that the border-button resides
    /// @return A list of points, which is the vertices of the mask polygon
    /// @note The generated point coordinates are relative to this panel
    QVector<QPointF> genBorderButtonMask(quint8 tSlot);

signals:
    void pSlotChanged();

private slots:
    void addPanel(quint8 tSlot);
    void delPanel(quint8 tSlot);
    void copyStyle(ConfigManager::Slot slot);

private:
    /// @brief One global config for all panels
    static ConfigManager *config;

    /// @brief The grid system to track all panels' locations
    /// @details
    /// ```````````````````````````````
    /// | ---• -1,2  •---•  1,1  •--- |
    /// |     \     /     \     /     |
    /// | -2,2 •---•  0,1  •---•  2,0 |
    /// |     /     \     /     \     |
    /// | ---• -1,1  •---•  1,0  •--- |
    /// |     \     /     \     /     |
    /// | -2,1 •---• (0,0) •---• 2,-1 |
    /// |     /     \     /     \     |
    /// | ---• -1,0  •---• 1,-1  •--- |
    /// |     \     /     \     /     |
    /// | -2,0 •---• 0,-1  •---• 2,-2 |
    /// |     /     \     /     \     |
    /// | ---• -1,-1 •---• 1,-2  •--- |
    /// ```````````````````````````````
    static QHash<QPoint, Panel *> panelGrid;

    /// @brief Coordinate in #panelGrid
    /// @see panelGrid
    QPoint coordinate;

    /// @brief Panel slot.
    /// @details Root panel has pSlot of 0. First-level panels (panel opened
    /// directly from root panel) has pSlot = tSlot + 1. Second and
    /// above-level panels has pSlot = parentPanel->pSlot + 6
    quint8 pSlot;

    /// @brief Parent panel of this panel
    Panel *parentPanel;

    /// @brief The tSlot of the parent panel in which this panel resides
    quint8 tSlot;

    /// @brief Children panels of this panel
    QVector<QSharedPointer<Panel>> childPanels;

    /// @brief Border buttons of this panel, for expanding children panels
    QVector<QSharedPointer<HiddenButton>> borderButtons;

    /// @brief How much should the button scale on mouse hover
    const qreal hoverScale;

    /// @brief Radius of the main hexagon (edge length)
    /// @details Must be signed since negative computations are involved
    const qreal unitLen;

    /// @brief gap between buttons
    /// @details Must be signed since negative computations are involved
    const qreal gapLen;
};
#endif // PANEL_H
