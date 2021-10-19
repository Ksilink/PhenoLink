add_definitions(-DExportGuiSymbols)
add_definitions(-DQHTTP_MEMORY_LOG=0)
add_definitions(-DQHTTP_HAS_CLIENT)

set(CheckoutGui_SRC
                mainwindow.cpp mainwindow_wellplate.cpp
                mainwindow_datatables.cpp mainwindow_startprocesses.cpp
                mainwindow_overlay.cpp
                mainwindow.h  mainwindow.ui
                guiserver.cpp guiserver.h
                screensmodel.cpp  screensmodel.h
                experimentworkbenchcontrol.cpp experimentworkbenchcontrol.h

                optioneditor.cpp optioneditor.h
                dashoptions.cpp dashoptions.h dashoptions.ui

                ImageDisplay/graphicssignitem.cpp ImageDisplay/graphicssignitem.h
                ImageDisplay/imageform.cpp ImageDisplay/imageform.h ImageDisplay/imageform.ui
                ImageDisplay/imagegraphicsview.cpp ImageDisplay/imagegraphicsview.h
                ImageDisplay/graphicspixmapitem.cpp ImageDisplay/graphicspixmapitem.h
                ImageDisplay/scrollzone.cpp ImageDisplay/scrollzone.h
                ImageDisplay/flowlayout.cpp ImageDisplay/flowlayout.h

                ScreensDisplay/graphicsscreensitem.cpp ScreensDisplay/graphicsscreensitem.h
                ScreensDisplay/screensgraphicsview.cpp ScreensDisplay/screensgraphicsview.h
                ScreensDisplay/wellrepresentationitem.cpp ScreensDisplay/wellrepresentationitem.h
                ScreensDisplay/wellcolorisation.cpp ScreensDisplay/wellcolorisation.h

                ctkWidgets/ctkValueProxy.cpp   ctkWidgets/ctkValueProxy.h
                ctkWidgets/ctkRangeSlider.cpp   ctkWidgets/ctkRangeSlider.h
                ctkWidgets/ctkDoubleRangeSlider.cpp   ctkWidgets/ctkDoubleRangeSlider.h
                ctkWidgets/ctkDoubleSlider.cpp ctkWidgets/ctkDoubleSlider.h

                ctkWidgets/ctkBasePopupWidget.cpp   ctkWidgets/ctkBasePopupWidget.h ctkWidgets/ctkBasePopupWidget_p.h
                ctkWidgets/ctkPopupWidget.cpp   ctkWidgets/ctkPopupWidget.h ctkWidgets/ctkPopupWidget_p.h

                ctkWidgets/ctkProxyStyle.cpp   ctkWidgets/ctkProxyStyle.h
                ctkWidgets/ctkCollapsibleGroupBox.cpp   ctkWidgets/ctkCollapsibleGroupBox.h

                ctkWidgets/ctkCheckableHeaderView.cpp ctkWidgets/ctkCheckableHeaderView.h
                ctkWidgets/ctkCheckBoxPixmaps.cpp ctkWidgets/ctkCheckBoxPixmaps.h
                ctkWidgets/ctkCheckableModelHelper.cpp ctkWidgets/ctkCheckableModelHelper.h

                ctkWidgets/ctkColorPickerButton.cpp ctkWidgets/ctkColorPickerButton.h
                ctkWidgets/ctkColorDialog.h ctkWidgets/ctkColorDialog.cpp

               ctkWidgets/ctkPathListWidget.h ctkWidgets/ctkPathListWidget.cpp
               ctkWidgets/ctkPathListButtonsWidget.ui ctkWidgets/ctkPathListButtonsWidget.h ctkWidgets/ctkPathListButtonsWidget_p.h ctkWidgets/ctkPathListButtonsWidget.cpp

                ctkWidgets/ctkPathLineEdit.h ctkWidgets/ctkPathLineEdit.cpp
#                ctkWidgets/ctkPathLineEditEventPlayer.h ctkWidgets/ctkPathLineEditEventPlayer.cpp
#                ctkWidgets/ctkPathLineEditEventTranslator.h ctkWidgets/ctkPathLineEditEventTranslator.cpp
                ctkWidgets/ctkCheckableComboBox.h ctkWidgets/ctkCheckableComboBox.cpp
               # ctkWidgets/ctkCheckableComboBoxEventPlayer.h ctkWidgets/ctkCheckableComboBoxEventPlayer.cpp
               # ctkWidgets/ctkCheckableComboBoxEventTranslator.h ctkWidgets/ctkCheckableComboBoxEventTranslator.cpp





                rcs.qrc
    )

if(VTK_FOUND)
    link_directories(${VTK_LIB_DIR})
    include(${VTK_USE_FILE})
    set(CheckoutVTKLib ${VTK_LIBRARIES})
endif(VTK_FOUND)

if (BUILD_PYTHON_SCRIPTING)
    set(CheckoutPythonLib CheckoutPythonCore)
endif(BUILD_PYTHON_SCRIPTING)

add_library(CheckoutGui SHARED ${CheckoutGui_SRC} )

QT5_WRAP_UI(mainwindow.ui ctkWidgets/ctkPathListButtonsWidget.ui)


#add_custom_command(TARGET CheckoutGui   PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
#    )

target_include_directories(CheckoutGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


IF(WIN32)
    target_link_libraries(CheckoutGui  ${CheckoutPythonLib} CheckoutCore ${CheckoutVTKLib} QHttp Qt::OpenGL Qt::Widgets Qt::Sql  Qt::WinExtras Qt::Charts Qt::WebEngineWidgets  ${OpenCV_LIBS} )
ELSE()
target_link_libraries(CheckoutGui  ${CheckoutPythonLib} CheckoutCore ${CheckoutVTKLib} QHttp Qt::OpenGL Qt::Widgets Qt::Sql ${OpenCV_LIBS} )
ENDIF(WIN32)



install(TARGETS CheckoutGui COMPONENT applications DESTINATION bin)
