#include "AppController.h"
#include "../including/AppData.h"
#include "../Wrap/Log.h"
#include "../Wrap/UserData.h"
#include "../../appcontroller/Appcontroller/appcont.h"
#include <stdlib.h>
#include <stdio.h>

#include <iostream>

namespace modules {
////////////////////////////////////

void* AppController(void *appData)
{
    app::AppData *data = (app::AppData*)appData;
    app::AppMessage &dataMsg = data->GetMsg();
    HandlerAppController handler(data);

    int selfID;

    if ((selfID = AppControllerInit(&handler, &dataMsg)))
    {
        app::Message msg;
        msg.SetRcv(selfID);
        app::MsgError err;

        bool continuation = true;

        do
        {
            msg.SetRcv(selfID);
            dataMsg.GetMessage(msg, err);

            if (err != app::ErrorNot)
            {
                wrap::Log("Error get message\n", LOGAPPCONTROLLER);
                continue;
            }
            else
            {
                continuation = handler.handler(msg);
            }

        } while(continuation);
    }
}

HandlerAppController::HandlerAppController(app::AppData *appData) : dataMsg(appData->GetMsg())
{
    data = appData;

    selfID = app::appController;
}

bool HandlerAppController::handler(app::Message &event)
{
    const char *textMsg = event.GetBodyMsg();
    std::string bodyMsg = textMsg;

    if(!bodyMsg.compare(QUIT))
    {
        dataMsg.DeleteModule(selfID);
        if (!selfID)
        {
            return false;
        }
        else
        {
            if (event.GetSnd() == app::controller)
            {
                event.SetBodyMsg("close appcontroller\n");
                event.SetRcv(app::controller);
                event.SetSnd(app::appController);

                app::MsgError err;
                dataMsg.AddMessage(event, err);
            }
        }

        return false;
    }

    if (AppCont(event))
    {
        std::cout << "Получение аутентификационных данных пользователя " << std::endl;
        std::cout.flush();

        const char* str = event.GetBodyMsg();

        wrap::UserDataAdd *usrData = (wrap::UserDataAdd*)str;

        std::cout << usrData->GetSock() << std::endl;
        std::cout.flush();
        app::MsgError err;
        event.SetRcv(event.GetSnd());
        event.SetSnd(selfID);
        dataMsg.AddMessage(event, err);
        return true;
    }
    else
    {
        wrap::Log("Error parse message\n", LOGAPPCONTROLLER);
        return true;
    }

    return true;
}

inline void HandlerAppController::SetSelfID(int id)
{
    selfID = id;
}

inline void HandlerAppController::SetNetID(int id)
{
    netID = id;
}

int AppControllerInit(HandlerAppController* handler, app::AppMessage *dataMsg)
{
    app::Message msg;
    msg.SetRcv(app::NewAppModule);
    app::MsgError err;

    dataMsg->GetMessage(msg, err);

    const char* textMsg = msg.GetBodyMsg();
    std::string bodyMsg = textMsg;

    if (msg.GetSnd() == app::controller)
    {
        msg.CreateMessage("Close AppController\n", app::controller, app::NewAppModule);
        dataMsg->AddMessage(msg, err);
        return false;
    }

    int newID = atoi(bodyMsg.c_str());
    handler->SetSelfID(newID);

    dataMsg->GetMessage(msg, err);

    if (msg.GetSnd() == app::controller)
    {
        msg.CreateMessage("Close AppController\n", app::controller, app::NewAppModule);
        dataMsg->AddMessage(msg, err);
        return false;
    }

    bodyMsg = msg.GetBodyMsg();

    int newNetID = atoi(bodyMsg.c_str());

    bodyMsg.erase();
    char buf[10];
    sprintf(buf, "%d", newID);
    bodyMsg.append(buf);

    msg.CreateMessage(bodyMsg.c_str(), newNetID, newID);

    dataMsg->AddMessage(msg, err);

    std::cout << "инициализация модуля appcontroller завершена успешна" << std::endl;
    std::cout.flush();

    return newID;
}

////////////////////////////////////
}





































