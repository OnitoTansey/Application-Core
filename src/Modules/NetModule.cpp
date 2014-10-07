#include <unistd.h>
#include "NetModule.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "../Wrap/WrapNet.h"

#include <iostream>

namespace modules {
////////////////////////////////////

void* NetModule(void *appData)
{
    app::AppData *data = (app::AppData*)appData;
    app::AppMessage &dataMsg = data->GetMsg();
    app::AppSetting dataSttng;
    dataSttng = data->GetSttng();

    NetData netData;

    InitNetModule(netData, dataMsg, dataSttng);

    Net net(&netData, data);

    net.Process();
}

Net::Net(NetData* net, app::AppData *data)
{
    netData = net;
    appData = data;

    userData = new UserData[netData->maxUser];    
    waitAuthUser = new int[netData->maxUser];

    for(int i=0; i<netData->maxUser; i++)
    {
        userData[i].sock = -1;
        waitAuthUser[i] = -1;
    }
    currentNumberUser = 0;
    maxIdUser = 0;
    maxWaitUser = 0;
}

Net::~Net()
{
    delete[] userData;
    delete[] waitAuthUser;
}

void Net::Process()
{
    sockaddr cliaddr;
    socklen_t clilen;
    fd_set rset;
    int connfd;
    int nReady;
    int nRcv;
    int maxfd;

    FD_ZERO(&rset);
    FD_SET(netData->socket, &rset);
    FD_SET(netData->pipe, &rset);
    maxfd = std::max(netData->socket, netData->pipe)+1;
    while(1)
    {
        nReady = wrap::Select(maxfd, &rset, NULL, NULL, NULL);
        if (nReady == -1 && errno == EINTR)
        {
            FD_ZERO(&rset);
            FD_SET(netData->socket, &rset);
            FD_SET(netData->pipe, &rset);
            for (int i=0; i<maxIdUser; i++)
            {
                if (userData[i].sock != -1)
                    FD_SET(userData[i].sock, &rset);
                if (waitAuthUser[i] != -1)
                    FD_SET(waitAuthUser[i], &rset);
            }

            continue;
        }

        if (FD_ISSET(netData->socket, &rset))
        {
            clilen = sizeof(cliaddr);
            while((connfd = accept(netData->socket, &cliaddr, &clilen) == -1))
                if (errno == EINTR)
                    continue;

            if (connfd != -1 && (errno != EAGAIN || errno != EWOULDBLOCK || errno != ECONNABORTED))
            {
                int i;
                for (i=0; i<netData->maxUser; i++)
                {
                    if (waitAuthUser[i] == -1)
                        waitAuthUser[i] = connfd;
                }

                if (i == netData->maxUser)
                {
                    while (!wrap::Close(connfd) && errno == EINTR);
                }
                else
                {
                    FD_SET(connfd, &rset);
                    if (i > maxWaitUser)
                        maxWaitUser = i;
                    if (connfd > maxfd)
                        maxfd = connfd;
                }

                if (--nReady<=0)
                    continue;
            }
        }
    }
}

void InitNetModule(NetData& netData, app::AppMessage& dataMsg, app::AppSetting& dataSttng)
{
    app::SettingData sttngData;
    dataSttng.GetSetting(sttngData);
    netData.maxUser = sttngData.GetUserOnThread();
    netData.ratio = sttngData.GetRatioAppContAppNet();
    netData.minUser = sttngData.GetMinUserOnThread();
    netData.createThread = false;

    app::Message msg;
    msg.SetRcv(app::NewNetModule);
    app::MsgError err;
    dataMsg.GetMessage(msg, err);

    std::string bodyMsg = msg.GetBodyMsg();

    size_t first;
    size_t second;

    netData.pipe = atoi(bodyMsg.substr(first = bodyMsg.find(DATASTART)+6, second = bodyMsg.find("//")).c_str());

    bodyMsg.erase(first, (second+2)-first);

    netData.id = atoi(bodyMsg.substr(first = bodyMsg.find(DATASTART)+6, second = bodyMsg.find("//")).c_str());

    bodyMsg.erase(first, (second+2)-first);

    netData.socket = atoi(bodyMsg.substr(first = bodyMsg.find(DATASTART)+6, second = bodyMsg.find("//")).c_str());

    bodyMsg.erase(first, (second+2)-first);

    netData.numberThread = atoi(bodyMsg.substr(first = bodyMsg.find(DATASTART)+6, second = bodyMsg.find("//")).c_str());

    bodyMsg.erase(first, (second+2)-first);

    netData.currentThread = atoi(bodyMsg.substr(first = bodyMsg.find(DATASTART)+6, second = bodyMsg.find("//")).c_str());

    msg.SetRcv(netData.id);

    char buff[10];

    read(netData.pipe, buff, 10);

    dataMsg.GetMessage(msg, err);

    bodyMsg = msg.GetBodyMsg();

    netData.appControllerID = atoi(bodyMsg.substr(bodyMsg.find(DATASTART)+6, bodyMsg.find(DATAEND)).c_str());
}

bool CheckRequest(std::string bodyMsg)
{
    size_t start1;
    size_t end1;
    size_t start2;
    size_t end2;

    start1 = bodyMsg.find(DATASTART);
    end1 = bodyMsg.rfind(DATAEND);

    if(start1 != std::string::npos && end1 != std::string::npos)
        bodyMsg.erase(start1+6, end1-start1-6);
    else
        return false;

    //<message><nameRcv>Onito</nameRcv><nameSnd>Kira</nameSnd><event>add</event><data>good job!</data><idNode>5</idNode></message>

    start2 = bodyMsg.find(MSGSTART);
    end2 = bodyMsg.find(MSGEND, start2);

    if (start2 != bodyMsg.rfind(MSGSTART) || start2 != 0 || start2 == std::string::npos || end2 == std::string::npos || (end2+10) != bodyMsg.length())
        return false;

    start1 = bodyMsg.find(NAMERCVSTART, start2+9);
    end1 = bodyMsg.find(NAMERCVEND, start1);

    if (start2+9 != bodyMsg.rfind(NAMERCVSTART) || start1 == std::string::npos || end1 == std::string::npos || end1 != bodyMsg.rfind(NAMERCVEND))
        return false;

    start2 = bodyMsg.find(NAMESNDSTART, end1+10);
    end2 = bodyMsg.find(NAMESNDEND, start2);

    if (end1+10 != bodyMsg.rfind(NAMESNDSTART) || start2 == std::string::npos || end2 == std::string::npos || end2 != bodyMsg.rfind(NAMESNDEND))
        return false;

    start1 = bodyMsg.find(EVENTSTART, end2+10);
    end1 = bodyMsg.find(EVENTEND, start1);

    if (end2+10 != bodyMsg.rfind(EVENTSTART) || start1 == std::string::npos || end1 == std::string::npos || end1 != bodyMsg.rfind(EVENTEND))
        return false;

    start2 = bodyMsg.find(DATASTART, end1+8);
    end2 = bodyMsg.find(DATAEND, start2);

    if (end1+8 != bodyMsg.rfind(DATASTART) || start2 == std::string::npos || end2 == std::string::npos || end2 != bodyMsg.rfind(DATAEND))
        return false;

    start1 = bodyMsg.find(IDNODESTART, end2+7);
    end1 = bodyMsg.find(IDNODEEND, start1);

    if (end2+7 != bodyMsg.rfind(IDNODESTART) || start1 == std::string::npos || end1 == std::string::npos || end1 != bodyMsg.rfind(IDNODEEND) || end1+19 != bodyMsg.length())
        return false;

    return true;
}

////////////////////////////////////
}
