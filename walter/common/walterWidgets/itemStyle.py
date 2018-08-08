"""A style that removes everything from a Tree View. It's copied from MASH."""

from .Qt import QtCore
from .Qt import QtGui
from .Qt import QtWidgets


class ItemStyle(QtWidgets.QCommonStyle):
    """
    This class defines the view item style and is only used when style sheets
    and the delegate are not sufficient.
    """

    # Constants
    DROP_INDICATOR_COLOR = QtGui.QColor(255, 255, 255)
    DROP_INDICATOR_WIDTH = 3
    DROP_INDICATOR_LEFT_OFFSET = -25

    def __init__(self, parent):
        """Called after the instance has been created."""
        self.__parent = parent
        super(ItemStyle, self).__init__()

    def drawComplexControl(self, control, option, painter, widget=None):
        """
        Draw the given control using the provided painter with the style
        options specified by option.
        """
        return self.__parent.drawComplexControl(
            control, option, painter, widget)

    def drawControl(self, element, option, painter, widget=None):
        """
        Draw the given element with the provided painter with the style options
        specified by option.
        """
        return self.__parent.drawControl(element, option, painter, widget)

    def drawItemPixmap(self, painter, rectangle, alignment, pixmap):
        """
        Draw the given pixmap in the specified rectangle, according to the
        specified alignment, using the provided painter.
        """
        return self.__parent.drawItemPixmap(
            painter, rectangle, alignment, pixmap)

    def drawItemText(
            self,
            painter,
            rectangle,
            alignment,
            palette,
            enabled,
            text,
            textRole=QtGui.QPalette.NoRole):
        """
        Draw the given text in the specified rectangle using the provided
        painter and palette.
        """
        return self.__parent.drawItemText(
            painter, rectangle, alignment, palette, enabled, text, textRole)

    def drawPrimitive(self, element, option, painter, widget=None):
        """
        Draw the given primitive element with the provided painter using the
        style options specified by option.
        """
        # Changes the way the drop indicator is drawn
        if element == QtWidgets.QStyle.PE_IndicatorItemViewItemDrop:
            if (not option.rect.isNull() and
                    widget and
                    widget.customIndicatorShown):
                painter.save()
                painter.setRenderHint(QtGui.QPainter.Antialiasing, True)
                oldPen = painter.pen()
                painter.setPen(
                    QtGui.QPen(
                        self.DROP_INDICATOR_COLOR, self.DROP_INDICATOR_WIDTH))
                rect = option.rect
                rect.setLeft(self.DROP_INDICATOR_WIDTH)
                rect.setRight(widget.width() - 2 * self.DROP_INDICATOR_WIDTH)
                if option.rect.height() == 0:
                    painter.drawLine(rect.topLeft(), option.rect.topRight())
                else:
                    painter.drawRect(rect)
                painter.setPen(oldPen)
                painter.restore()

    def generatedIconPixmap(self, iconMode, pixmap, option):
        """
        Return a copy of the given pixmap, styled to conform to the specified
        iconMode and taking into account the palette specified by option.
        """
        return self.__parent.generatedIconPixmap(iconMode, pixmap, option)

    def hitTestComplexControl(self, control, option, position, widget=None):
        """
        Return the sub control at the given position in the given complex
        control (with the style options specified by option).
        """
        return self.__parent.hitTestComplexControl(
            control, option, position, widget)

    def itemPixmapRect(self, rectangle, alignment, pixmap):
        """
        Return the area within the given rectangle in which to draw the
        specified pixmap according to the defined alignment.
        """
        return self.__parent.itemPixmapRect(rectangle, alignment, pixmap)

    def itemTextRect(self, metrics, rectangle, alignment, enabled, text):
        """
        Return the area within the given rectangle in which to draw the provided
        text according to the specified font metrics and alignment. The enabled
        parameter indicates whether or not the associated item is enabled.
        """
        return self.__parent.itemTextRect(
            metrics, rectangle, alignment, enabled, text)

    def pixelMetric(self, metric, option=None, widget=None):
        """Return the value of the given pixel metric."""
        return self.__parent.pixelMetric(metric, option, widget)

    def polish(self, *args, **kwargs):
        """Initialize the appearance of the given widget."""
        return self.__parent.polish(*args, **kwargs)

    def styleHint(self, hint, option=None, widget=None, returnData=None):
        """
        Return an integer representing the specified style hint for the given
        widget described by the provided style option.
        """
        if hint == QtWidgets.QStyle.SH_Slider_AbsoluteSetButtons:
            return \
                QtCore.Qt.LeftButton | \
                QtCore.Qt.MidButton | \
                QtCore.Qt.RightButton
        return self.__parent.styleHint(hint, option, widget, returnData)

    def subControlRect(self, control, option, subControl, widget=None):
        """
        Return the rectangle containing the specified subControl of the given
        complex control (with the style specified by option). The rectangle is
        defined in screen coordinates.
        """
        return self.__parent.subControlRect(control, option, subControl, widget)

    def subElementRect(self, element, option, widget=None):
        """
        Return the sub-area for the given element as described in the provided
        style option. The returned rectangle is defined in screen coordinates.
        """
        return self.__parent.subElementRect(element, option, widget)

    def unpolish(self, *args, **kwargs):
        """Uninitialize the given widget's appearance."""
        return self.__parent.unpolish(*args, **kwargs)

    def sizeFromContents(self, ct, opt, contentsSize, widget=None):
        """
        Return the size of the element described by the specified option and
        type, based on the provided contentsSize.
        """
        return self.__parent.sizeFromContents(ct, opt, contentsSize, widget)
# ===========================================================================
# Copyright 2016 Autodesk, Inc. All rights reserved.
#
# Use of this software is subject to the terms of the Autodesk license
# agreement provided at the time of installation or download, or which
# otherwise accompanies this software in either electronic or hard copy form.
# ===========================================================================
