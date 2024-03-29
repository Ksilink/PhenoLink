/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

#ifndef __ctkCheckableComboBoxEventTranslator_h
#define __ctkCheckableComboBoxEventTranslator_h

// QT includes
#include "QModelIndexList"

// QtTesting inlcudes
#include <pqWidgetEventTranslator.h>

// CTK includes
#include <ctkPimpl.h>
#include <Core/Dll.h>


class CTK_WIDGETS_EXPORT ctkCheckableComboBoxEventTranslator :
  public QObject
{
  Q_OBJECT

public:
  ctkCheckableComboBoxEventTranslator(QObject* parent = 0);

  virtual bool translateEvent(QObject *Object, QEvent *Event, bool &Error);

private:
  Q_DISABLE_COPY(ctkCheckableComboBoxEventTranslator);

  QObject* CurrentObject;
  QModelIndexList OldIndexList;

private slots:
  void onDestroyed(QObject*);
  void onStateChanged(const QString& State);
  void onDataChanged(const QModelIndex&, const QModelIndex&);
};

#endif
