#include "Controller.h"
#include "../including/AppData.h"
#include "../Wrap/Log.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "errno.h"
#include <iostream>

namespace modules {
////////////////////////////////////

void* Controller(void *appData)
{
    app::AppData *data= (app::AppData *)appData;
    app::AppMessage &dataMsg = data->GetMsg();

    UILink linkUI;
    InitUILink(&dataMsg, &linkUI);

    ControllerHandler handler(&dataMsg, &linkUI);

    app::Message msg;
    msg.SetRcv(app::controller);
    app::MsgError err;

    bool continuation = true;

    do
    {
        msg.SetRcv(app::controller);
        dataMsg.GetMessage(msg, err);

        if (err != app::ErrorNot)
        {
            wrap::Log("Error get message\n", LOGCONTROLLER);
            continue;
        }
        else
        {
            continuation = handler.Handler(&msg);
        }

    } while(continuation);
}

int UILink::GetLink()
{
    return sockUI;
}

void UILink::SetLink(int link)
{
    sockUI = link;
}

void InitUILink(app::AppMessage* dataMsg, UILink* link)
{
    app::Message msg;
    app::MsgError err;

    msg.SetRcv(app::controller);
    dataMsg->GetMessage(msg, err);

    if (err == app::ErrorNot)
        link->SetLink(atoi(msg.GetBodyMsg()));
    else
    {
        wrap::Log("Error set link to UI\n", LOGCONTROLLER);
        exit(1);
    }
}

ControllerBehavior::ControllerBehavior()
{
    for(int i=FIRSTEVENTS; i<EVENTLAST; i++)
    {
        condition[i] = UnavailableEvent;
    }
}

void ControllerBehavior::SetBehavior(int event)
{
    if (FIRSTEVENTS<=event && event<EVENTLAST)
    {
        condition[event] = (Event)event;
    }
}

bool ControllerBehavior::TestSetEvent(int event)
{
    if (FIRSTEVENTS<=event && event<EVENTLAST)
    {
        return (condition[event]==event);
    }
    return false;
}

int ControllerHandlerData::GetHandlerEvent()
{
    return event;
}

std::string ControllerHandlerData::GetHandlerParam()
{
    return param;
}

void ControllerHandlerData::SetHandlerEvent(int evnt)
{
    event = evnt;
}

void ControllerHandlerData::SetHandlerParam(std::string parm)
{
    param = parm;
}

ControllerHandler::ControllerHandler(app::AppMessage *appData, UILink *uiLink)
{
    dataMsg = appData;
    linkUI = uiLink;
    for(int i=FIRSTEVENTS; i<EVENTLAST; i++)
    {
        behavior[i].SetBehavior(i);
    }
}

ControllerHandler::~ControllerHandler()
{

}

void ControllerHandler::Parser(std::string event, ControllerHandlerData* handlerData)
{
    std::string param;

    size_t positionParam;

    if (event.find(EXIT) != std::string::npos)
    {
        handlerData->SetHandlerEvent(APPEXIT);
        return;
    }
    else if (event.find(LOG) != std::string::npos)
    {
        if ((positionParam = event.find("-")) != std::string::npos)
        {
            handlerData->SetHandlerEvent(-1);
            return;
        }

        param = event.substr(positionParam);

        handlerData->SetHandlerParam(param);

        handlerData->SetHandlerEvent(LOGDATA);

        return;
    }
    else if (event.find(SHOW) != std::string::npos)
    {
        if ((positionParam = event.find("-")) != std::string::npos)
        {
            handlerData->SetHandlerEvent(-1);
            return;
        }

        param = event.substr(positionParam);

        handlerData->SetHandlerParam(param);

        handlerData->SetHandlerEvent(SHOWDATA);

        return;
    }

    handlerData->SetHandlerEvent(-1);
}

bool ControllerHandler::EventHandler(int behaviorEvent, std::string param)
{
    if (behavior[behaviorEvent].TestSetEvent(APPEXIT))
    {
        app::Message msg;
        msg.CreateMessage("quit", app::UnavailableName, app::controller);
        app::MsgError err;

        int countModules = dataMsg->AddMessageAllModules(msg, err);
        msg.SetRcv(app::controller);
        for (int i=0; i<countModules-BASICMODULES; i++)
        {
            dataMsg->GetMessage(msg, err);
            wrap::Log(msg.GetBodyMsg(), LOGCONTROLLER);
            std::cout << msg.GetBodyMsg() << msg.GetSnd() << std::endl;
        }
        return false;
    }
    else if (behavior[behaviorEvent].TestSetEvent(LOGDATA))
    {
        wrap::Log(param.c_str(), LOGCONTROLLER);
        return true;
    }
    else if (behavior[behaviorEvent].TestSetEvent(SHOWDATA))
    {
        int nRcv;
        while (1)
        {
            nRcv = write(linkUI->GetLink(), param.c_str(), param.length());
            if ((nRcv != -1) && (nRcv != 0))
                break;
        }
        return true;
    }
    return true;
}

bool ControllerHandler::Handler(app::Message* msg)
{
    ControllerHandlerData handlerData;

    Parser(msg->GetBodyMsg(), &handlerData);

    if (handlerData.GetHandlerEvent() == -1)
    {
        wrap::Log("incorrect command\n", LOGCONTROLLER);
        return true;
    }

    if (EventHandler(handlerData.GetHandlerEvent(), handlerData.GetHandlerParam()))
    {
        return true;
    }
    else
    {
        wrap::Log("close app...\n", LOGCONTROLLER);
        exit(0);
        return false;
    }
}

////////////////////////////////////
}
