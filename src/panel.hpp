#ifndef PANEL_H
#define PANEL_H

#include "button.hpp"
#include "buttoninfo.hpp"
#include "configs.hpp"
#include "hiddenbutton.hpp"

#include <QPushButton>
#include <QSharedPointer>
#include <QStack>
#include <QVector>
#include <QWeakPointer>
#include <QWidget>
#include <ResvgQt.h>
#include <memory>

/// @brief A Panel is a hexagon that contains multiple buttons.
class Panel : public QWidget {
    Q_OBJECT
public:
    Panel(
        Panel *parent = nullptr, quint8 tSlot = 0,
        const QSharedPointer<Configs> &configs = nullptr);

public slots:
    void copyStyle();

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
    void delStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief Add button that fills border of the hexagon
    /// @param tSlot theta(angle)-slot, range from 0-5
    HiddenButton *addBorderButton(quint8 tSlot);
    void delBorderButton(quint8 tSlot);

    /// @brief Redraw central button according to #composedStyles
    void updateCentralButton();

    /// @brief Update #composedStyles from #activeButtons
    void composeCentralButtonInfo();

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

    /// @brief Generate mask for a border-button
    /// @param tSlot @see addStyleButton
    /// @param rSlot @see addStyleButton
    /// @param subSlot @see addStyleButton
    /// @return A list of points, which is the vertices of the mask polygon
    /// @note The generated point coordinates are relative to this panel
    QVector<QPointF>
    genStyleButtonMask(quint8 tSlot, quint8 rSlot, quint8 subSlot);

    QVector<QPointF> genCentralButtonMask();

    QPixmap
    drawStyleButtonIcon(quint8 tSlot, quint8 rSlot, quint8 subSlot) const;

    QPixmap drawCentralButtonIcon() const;

    /// @brief Tells whether this panel is currently active.
    /// @details A panel is active if one of its button is active, or one of
    /// its child panel is active. An active panel should not be automatically
    /// closed when lose focus.
    bool isActive() const;

    /// @brief Get style from clipboard and store it to the config file
    void updateStyleFromClipboard(quint8 tSlot, quint8 rSlot, quint8 subSlot);

    static Configs::Slot
    calcSlot(quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief Generate options to pass to resvg renderer.
    /// @return The returned optios should not be copied.
    static const ResvgOptions &genResvgOptions();

private slots:
    void addPanel(quint8 tSlot);
    void delPanel(quint8 tSlot);

private:
    /// @brief A config that is shared across all panels
    QSharedPointer<Configs> configs;

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
    typedef QHash<QPoint, Panel *> PGrid;
    /// @brief The panel grid storage. Use the alias #panelGrid instead.
    /// @brief This member is shared by all panels in the grid.
    QSharedPointer<PGrid> _pGrid;
    /// @brief A convenient alias to (*_pGrid)
    PGrid &panelGrid;

    /// @brief Coordinate in #panelGrid
    /// @see panelGrid
    QPoint coordinate;

    /// @brief Panel slot.
    /// @details Root panel has pSlot of 0. First-level panels (panel opened
    /// directly from root panel) has pSlot = tSlot + 1. Second and
    /// above-level panels has pSlot = parentPanel->pSlot + 6
    const quint8 pSlot;

    /// @brief Parent panel of this panel
    Panel *const parentPanel;

    /// @brief The tSlot of the parent panel in which this panel resides
    quint8 tSlot;

    /// @brief Children panels of this panel
    QVector<QSharedPointer<Panel>> childPanels;

    /// @brief Style buttons, mapped to corresponding slot
    QHash<Configs::Slot, QSharedPointer<Button>> styleButtons;

    /// @brief Border buttons of this panel, for expanding children panels
    QVector<QSharedPointer<HiddenButton>> borderButtons;

    /// @brief The button at the very center
    QSharedPointer<Button> centralButton;

    /// @brief How much should the button scale on mouse hover
    const qreal hoverScale;

    /// @brief Radius of the main hexagon (edge length)
    /// @details Must be signed since negative computations are involved
    const qreal unitLen;

    /// @brief Gap between buttons
    /// @details Must be signed since negative computations are involved
    const qreal gapLen;

    /// @brief Record a list of active buttons
    /// @note The active buttons can reside on child panels of this panel.
    class ActiveButtons {
    public:
        /// @brief Try to append the button to the tail of the queue
        /// @details If the button already exists in the queue, do nothing
        void insert(const Configs::Slot &slot);
        /// @brief Remove a button from the queue
        void remove(const Configs::Slot &slot);
        /// @brief Return number of active buttons
        qsizetype size() const;
        /// @brief Return the queue
        QList<Configs::Slot> orderedList() const;

        ActiveButtons() = default;

    private:
        /// @brief stores {cur, {prev, next}}
        /// @details O(1) time for insert and remove while maintaining order.
        /// * If `prev == cur`, then `cur` is head
        /// * If `next == cur`, then `cur` is tail
        QHash<Configs::Slot, QPair<Configs::Slot, Configs::Slot>> list;
        /// @brief First active button in the deque.
        /// @details If list is empty, this value is meaningless.
        Configs::Slot head;
        /// @brief Last active button in the deque
        /// @details If list is empty, this value is meaningless.
        Configs::Slot tail;
    } activeButtons;

    /// @brief Styles composed from #activeButtons
    QSharedPointer<ButtonInfo> centralButtonInfo;
};
#endif // PANEL_H
