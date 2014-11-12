#ifndef NETMODULE_H
#define NETMODULE_H

#pragma once

#include <string>
#include <sys/poll.h>
#include <vector>
#include "../including/AppData.h"
#include <unordered_map>
#include <deque>
#include "ModulesDefined.h"

namespace modules {
////////////////////////////////////

struct NetData
{
    int pipe;               // пробуждает поток в случае появления для него сообщения внутри программы  //
    int id;                 // id потока в модуле AppMessage                                            //
    int socket;             // прослушивающий сокет                                                     //
    int numberThread;       // количество потоков на текущем appcontroller                              //
    int maxUser;            // максимальное число пользователей на поток                                //
    int ratio;              // соотношение количества потоков NetModule на AppController                //
    int appControllerId;    // id контроллера к которому привязан текущей модуль                        //
    int userOnchatRoom;     // число пользователей на одну комнату чата
    bool createThread;      // каждый поток может создать только одного потомка, это флаг               //
};

struct UserData
{
    std::string name;

    std::string rcvBuf;

    std::string sndBuf;

    int numberRoom;
    int posInRoom;

    int size;

    int readSize;
};

struct PrivateMsg
{
    unsigned int idUserSnd;
    unsigned int idUserRcv;

    std::string msg;

};

struct PublicMsg
{
    std::string idUserName;
    std::vector<int> halfTransmission;

    std::string msg;
};

struct ChatRoom
{
    unsigned int *usrID;
    unsigned int availableSpace;
    unsigned int numUsr;
    bool waitHandler;

    std::deque<PrivateMsg> queuePrivateMsg;

    std::deque<PublicMsg> queuePublicMsg;

    ChatRoom() : waitHandler(false) { }
    ~ChatRoom();

    void ChatRoomInit(int valUsr);

    void AddPrivatMsg(PrivateMsg msg);
    void AddPublicMsg(PublicMsg msg);

    bool CheckAvailableSpace();
    unsigned int AddUsr(int id);
};

class Chat
{
private:
    ChatRoom *chatRoom;
    unsigned int numRoom;

    std::deque<int> roomWaitHandler;
public:
    Chat(int valRoom, int numUsrOnRoom);
    ~Chat();

     void AddUsr(int id, unsigned int& posInRoom, unsigned int& numberRoom);
     std::deque<int>::iterator GetIDRoomWaitHadler(/*int* idRoom, unsigned int* size*/);
     ChatRoom* GetChatRoom(unsigned int num);
};

class Net
{
private:
    NetData* netData;
    UserData* userData;
    app::AppData* appData;
    Chat *chat;

    pollfd *client;
    app::AppMessage &appMsg;

    char buf[1000];

    int currentNumberUser;

    bool HandlerLocalMsg();
public:
    Net(NetData* net, app::AppData* data);
    ~Net();
    void Process();
    void CreateNewThread();
};

void* NetModule(void *appdata);

bool InitNetModule(NetData& netData, app::AppMessage& dataMsg, app::AppSetting& dataSttng);

bool CheckRequest(std::string bodyMsg);

////////////////////////////////////
}

#endif // NETMODULE_H
