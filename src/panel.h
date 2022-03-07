#ifndef PANEL_H
#define PANEL_H

#include "button.h"
#include "hiddenbutton.h"

#include <QPushButton>
#include <QSharedPointer>
#include <QStack>
#include <QVector>
#include <QWeakPointer>
#include <QWidget>

/// @brief A Panel is a hexagon that contains multiple buttons.
class Panel : public QWidget {
    Q_OBJECT
private:
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
    QPoint coordinate;

    /// @brief Calculate asbolute position of target panel (relative to this)
    /// @param[in] tSlot The target panel occupies tSlot of this panel
    /// @return Coordinate of the target panel
    inline QPoint calcRelativeCoordinate(quint8 tSlot);

    Panel *parentPanel;

    /// @brief The rslot of the parent panel in which this panel resides
    quint8 tSlot;
    QVector<QSharedPointer<Panel>> childPanels;
    QVector<QSharedPointer<HiddenButton>> borderButtons;

    const qreal hoverScale;

    /// @brief Calculate asbolute position of target panel
    /// @param[in] tSlot The target panel occupies tSlot of this panel
    /// @return Absolute position of the target panel
    inline QPoint calcRelativePanelPos(quint8 tSlot);

    /// @brief Generate mask for a border-button
    /// @param tSlot The theata-slot that the border-button resides
    /// @return A list of points, which is the verteces of the mask polygon
    /// @note The generated point coordinates are relative to this panel
    QVector<QPoint> genBorderButtonMask(quint8 tSlot);

    /// @brief update masked area
    void updateMask();

public:
    Panel(Panel *parent = nullptr, quint8 tSlot = 0);
    virtual ~Panel() override;

    /// @brief Radius of the main hexagon (edge length)
    qint32 unitLen;
    /// @brief gap/2 between buttons
    qint32 gapLen;

    void redraw();

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
    /// | Numbers in cells are subslots         |
    /// `````````````````````````````````````````
    Button *addStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief Add button that fills border of the hexagon
    /// @param tSlot theta(angle)-slot, range from 0-5
    HiddenButton *addBorderButton(quint8 tSlot);
    void delBorderButton(quint8 tSlot);

protected:
    /// @brief Overridden to recursively move all panels
    void moveEvent(QMoveEvent *event) override;
    /// @brief Overridden to recursively close all panels
    void closeEvent(QCloseEvent *event) override;

private slots:
    void addPanel(quint8 tSlot);
    void delPanel(quint8 tSlot);
    void copyStyle();
};
#endif // PANEL_H
