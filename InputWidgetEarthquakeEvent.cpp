// Written: fmckenna

/* *****************************************************************************
Copyright (c) 2016-2017, The Regents of the University of California (Regents).
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS 
PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, 
UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

*************************************************************************** */

// Written: fmckenna

#include "InputWidgetEarthquakeEvent.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QStackedWidget>
#include <QComboBox>


#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>

#include <QLabel>
#include <QLineEdit>
#include <QDebug>
#include <QFileDialog>
#include <QPushButton>
#include <sectiontitle.h>
//#include <InputWidgetEDP.h>

#include <InputWidgetExistingEvent.h>
#include <ExistingSimCenterEvents.h>
#include <UniformMotionInput.h>
#include <ExistingPEER_Events.h>
#include "SHAMotionWidget.h"
#include <UserDefinedApplication.h>

InputWidgetEarthquakeEvent::InputWidgetEarthquakeEvent(RandomVariableInputWidget *theRandomVariableIW, QWidget *parent)
    : SimCenterAppWidget(parent), theCurrentEvent(0), theRandomVariableInputWidget(theRandomVariableIW)
{
    QVBoxLayout *layout = new QVBoxLayout();

    //
    // the selection part
    //

    QHBoxLayout *theSelectionLayout = new QHBoxLayout();
    QLabel *label = new QLabel();
    label->setText(QString("Loading Type"));
    eventSelection = new QComboBox();
    //    eventSelection->addItem(tr("Existing"));
    eventSelection->addItem(tr("Multiple Existing"));
    eventSelection->addItem(tr("Multiple PEER"));
    eventSelection->addItem(tr("Hazard Based Event"));
    eventSelection->addItem(tr("User Application"));
    eventSelection->setItemData(1, "A Seismic event using Seismic Hazard Analysis and Record Selection/Scaling", Qt::ToolTipRole);

    theSelectionLayout->addWidget(label);
    theSelectionLayout->addWidget(eventSelection);
    theSelectionLayout->addStretch();
    layout->addLayout(theSelectionLayout);

    //
    // create the stacked widget
    //
    theStackedWidget = new QStackedWidget();

    //
    // create the individual load widgets & add to stacked widget
    //

    //theExistingEventsWidget = new InputWidgetExistingEvent(theRandomVariableInputWidget);
    //theStackedWidget->addWidget(theExistingEventsWidget);
    theExistingEvents = new ExistingSimCenterEvents(theRandomVariableInputWidget);
    theStackedWidget->addWidget(theExistingEvents);

    theExistingPeerEvents = new ExistingPEER_Events(theRandomVariableInputWidget);
    theStackedWidget->addWidget(theExistingPeerEvents);

    //Adding SHA based ground motion widget
    theSHA_MotionWidget = new SHAMotionWidget(theRandomVariableInputWidget);
    theStackedWidget->addWidget(theSHA_MotionWidget);

    theUserDefinedApplication = new UserDefinedApplication(theRandomVariableInputWidget);
    theStackedWidget->addWidget(theUserDefinedApplication);

    layout->addWidget(theStackedWidget);
    this->setLayout(layout);
    theCurrentEvent=theExistingEvents;

    connect(eventSelection,SIGNAL(currentIndexChanged(QString)),this,SLOT(eventSelectionChanged(QString)));
}

InputWidgetEarthquakeEvent::~InputWidgetEarthquakeEvent()
{

}


bool
InputWidgetEarthquakeEvent::outputToJSON(QJsonObject &jsonObject)
{
    QJsonArray eventArray;
    QJsonObject singleEventData;
    theCurrentEvent->outputToJSON(singleEventData);
    eventArray.append(singleEventData);
    jsonObject["Events"]=eventArray;

    return true;
}


bool
InputWidgetEarthquakeEvent::inputFromJSON(QJsonObject &jsonObject) {

    QString type;
    QJsonObject theEvent;

    if (jsonObject.contains("Events")) {
        QJsonArray theEvents = jsonObject["Events"].toArray();
        QJsonValue theValue = theEvents.at(0);
        if (theValue.isNull()) {
          return false;
        }
        theEvent = theValue.toObject();
        if (theEvent.contains("type")) {
            QJsonValue theName = theEvent["type"];
            type = theName.toString();
        } else
            return false;
    } else
        return false;


    int index = 0;
    if ((type == QString("Existing Events")) || (type == QString("ExistingSimCenterEvents"))) {
        index = 0;
    } else if ((type == QString("Existing PEER Events")) || (type == QString("ExistingPEER_Events"))) {
        index = 1;
    } else if (type == QString("Hazard Besed Event")) {
        index = 2;
    } else if ((type == QString("User Application")) || (type == QString("UserDefinedApplication"))) {
        index = 3;
    } else {
        return false;
    }

    eventSelection->setCurrentIndex(index);

    // if worked, just invoke method on new type

    if (theCurrentEvent != 0) {
        return theCurrentEvent->inputFromJSON(theEvent);
    }

    return false;
}

void InputWidgetEarthquakeEvent::eventSelectionChanged(const QString &arg1)
{
    //
    // switch stacked widgets depending on text
    // note type output in json and name in pull down are not the same and hence the ||
    //

    if (arg1 == "Multiple Existing") {
        theStackedWidget->setCurrentIndex(0);
        theCurrentEvent = theExistingEvents;
    }

    else if(arg1 == "Multiple PEER") {
        theStackedWidget->setCurrentIndex(1);
        theCurrentEvent = theExistingPeerEvents;
    }

    else if(arg1 == "Hazard Based Event") {
        theStackedWidget->setCurrentIndex(2);
        theCurrentEvent = theSHA_MotionWidget;
    }

    else if(arg1 == "User Application") {
        theStackedWidget->setCurrentIndex(3);
        theCurrentEvent = theUserDefinedApplication;
    }

    else {
        qDebug() << "ERROR .. InputWidgetEarthquakeEvent selection .. type unknown: " << arg1;
    }
}

bool
InputWidgetEarthquakeEvent::outputAppDataToJSON(QJsonObject &jsonObject)
{
    QJsonArray eventArray;
    QJsonObject singleEventData;
    theCurrentEvent->outputAppDataToJSON(singleEventData);
    eventArray.append(singleEventData);
    jsonObject["Events"]=eventArray;

    return true;
}


bool
InputWidgetEarthquakeEvent::inputAppDataFromJSON(QJsonObject &jsonObject)
{

    QJsonObject theEvent;

    if (jsonObject.contains("Events")) {
        QJsonArray theEvents = jsonObject["Events"].toArray();
        QJsonValue theValue = theEvents.at(0);
        if (theValue.isNull()) {
          return false;
        }
        theEvent = theValue.toObject();
    } else
        return false;


    // if worked, just invoke method on new type

    if (theCurrentEvent != 0 && !theEvent.isEmpty()) {
        return theCurrentEvent->inputAppDataFromJSON(theEvent);
    }
}

bool
InputWidgetEarthquakeEvent::copyFiles(QString &destDir) {

    if (theCurrentEvent != 0) {
        return  theCurrentEvent->copyFiles(destDir);
    }

    return false;
}
