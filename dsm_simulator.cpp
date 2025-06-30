#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <thread>
#include <mutex>
#include <sstream>
#include <cstring>
#include <cassert>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

const int NUM_NODES = 4;
const int NUM_BLOCKS = 8;
using BlockID = int;
using NodeID = int;
using Value = int;

enum MessageType { INVALIDATE, FETCH_REQ, FETCH_RES, WRITE_CLAIM };

#pragma pack(push, 1)
struct Message {
    MessageType type;
    int block;
    int value;
    int src;
};
#pragma pack(pop)

struct DirectoryEntry {
    NodeID owner;
    std::bitset<NUM_NODES> sharers;
};

struct LocalBlock {
    bool valid = false;
    Value data = 0;
};

class DSMNode {
public:
    DSMNode(NodeID id, const std::string& cfg, const std::string& cmd)
        : id(id), directory(NUM_BLOCKS), cache(NUM_BLOCKS) {
        loadConfig(cfg);
        startServer();
        connectAll();
        loadCommands(cmd);
    }

    void run() {
        // launch a receiver thread per peer for true multithreading
        for (int i = 0; i < NUM_NODES; ++i) {
            if (i + 1 == id) continue;
            recvThreads.emplace_back(&DSMNode::peerReceiver, this, i);
        }
        // process commands, possibly in parallel
        for (auto& c : commands) {
            execThreads.emplace_back([this, &c]() {
                if (c.first == 'R') readBlock(c.second[0]);
                else writeBlock(c.second[0], c.second[1]);
            });
        }
        // join all threads
        for (auto& t : execThreads) t.join();
        for (auto& t : recvThreads) t.detach();
    }

private:
    NodeID id;
    int serverSock;
    std::vector<std::pair<std::string,int>> peers;
    std::vector<int> sockets;
    std::vector<DirectoryEntry> directory;
    std::vector<LocalBlock> cache;
    std::vector<std::pair<char,std::vector<int>>> commands;
    std::vector<std::thread> recvThreads;
    std::vector<std::thread> execThreads;
    std::mutex mtx;

    void loadConfig(const std::string& path) {
        std::ifstream in(path);
        assert(in);
        std::string ip; int port;
        while (peers.size() < NUM_NODES && in >> ip >> port)
            peers.emplace_back(ip,port);
        assert(peers.size()==NUM_NODES);
        sockets.resize(NUM_NODES,-1);
    }

    void startServer() {
        auto [ip,port] = peers[id-1];
        serverSock = socket(AF_INET,SOCK_STREAM,0);
        sockaddr_in addr{};
        addr.sin_family=AF_INET;
        addr.sin_addr.s_addr=inet_addr(ip.c_str());
        addr.sin_port=htons(port);
        bind(serverSock,(sockaddr*)&addr,sizeof(addr));
        listen(serverSock,NUM_NODES);
    }

    void connectAll() {
        // connect to lower IDs as client
        for(int i=0;i<NUM_NODES;i++){
            if(i+1==id) continue;
            if(i+1<id) {
                int s=socket(AF_INET,SOCK_STREAM,0);
                sockaddr_in addr{};
                addr.sin_family=AF_INET;
                addr.sin_addr.s_addr=inet_addr(peers[i].first.c_str());
                addr.sin_port=htons(peers[i].second);
                while(connect(s,(sockaddr*)&addr,sizeof(addr))<0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                sockets[i]=s;
            }
        }
        // accept higher IDs
        for(int i=0;i<NUM_NODES;i++){
            if(i+1==id) continue;
            if(i+1>id){
                int c=accept(serverSock,nullptr,nullptr);
                sockets[i]=c;
            }
        }
    }

    void loadCommands(const std::string& path) {
        std::ifstream in(path);
        assert(in);
        char op; int b,v;
        while(in>>op){
            if(op=='R'){in>>b; commands.emplace_back(op,vector<int>{b});}
            else{in>>b>>v;commands.emplace_back(op,vector<int>{b,v});}
        }
    }

    void peerReceiver(int idx) {
        int sock = sockets[idx];
        while(true) {
            Message msg;
            if(recv(sock,&msg,sizeof(msg),0)<=0) continue;
            handle(msg);
        }
    }

    void handle(const Message& msg) {
        std::lock_guard<std::mutex> lk(mtx);
        switch(msg.type) {
            case INVALIDATE:
                cache[msg.block].valid=false;
                break;
            case FETCH_REQ: {
                Message res{FETCH_RES,msg.block,cache[msg.block].data,id};
                send(sockets[msg.src-1],&res,sizeof(res),0);
                break; }
            case FETCH_RES:
                cache[msg.block].data=msg.value;
                cache[msg.block].valid=true;
                break;
            case WRITE_CLAIM:
                directory[msg.block].owner=msg.src;
                directory[msg.block].sharers.reset();
                directory[msg.block].sharers.set(msg.src-1);
                break;
        }
    }

    void broadcast(const Message& msg) {
        for(int i=0;i<NUM_NODES;i++) if(i+1!=id)
            send(sockets[i],&msg,sizeof(msg),0);
    }

    void readBlock(BlockID b) {
        if(!cache[b].valid){
            Message req{FETCH_REQ,b,0,id};
            send(sockets[directory[b].owner-1],&req,sizeof(req),0);
            while(!cache[b].valid) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::lock_guard<std::mutex> lk(mtx);
        std::cout<<"Node"<<id<<"READ"<<b<<"="<<cache[b].data<<"\n";
    }

    void writeBlock(BlockID b, Value v) {
        Message inv{INVALIDATE,b,0,id}; broadcast(inv);
        directory[b].owner=id;
        Message claim{WRITE_CLAIM,b,0,id}; broadcast(claim);
        {
            std::lock_guard<std::mutex> lk(mtx);
            cache[b].data=v;cache[b].valid=true;
        }
        std::cout<<"Node"<<id<<"WRITE"<<b<<"="<<v<<"\n";
    }
};

int main(int ac,char**av){assert(ac==4);
    DSMNode node(std::stoi(av[1]),av[2],av[3]); node.run();
}
