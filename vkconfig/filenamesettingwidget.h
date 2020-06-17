#ifndef CFILENAMESETTINGWIDGET_H
#define CFILENAMESETTINGWIDGET_H
/*
 * Copyright (c) 2020 Valve Corporation
 * Copyright (c) 2020 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This class creates an edit control that is paired with a setting
 * in a profile.
 * Author: Richard S. Wright Jr. <richard@lunarg.com>
 */


#include <QWidget>
#include <QTreeWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QFileDialog>

#include "layerfile.h"

class CFilenameSettingWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CFilenameSettingWidget(QTreeWidgetItem* pItem, TLayerSettings* pLayerSetting);

private:
    virtual void resizeEvent(QResizeEvent *event) override;

    TLayerSettings *pSetting;
    QLineEdit      *pLineEdit;
    QPushButton    *pPushButton;

public Q_SLOTS:
    void browseButtonClicked(void);
    void textFieldChanged(const QString& newText);

};

#endif // CFILENAMESETTINGWIDGET_H