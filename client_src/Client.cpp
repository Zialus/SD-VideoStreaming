// Ice includes
#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <IceUtil/IceUtil.h>

// C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>


#include <readline/readline.h>
#include <readline/history.h>
#include <err.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

// My stuff
#include "StreamServer.h"
#include "../auxiliary/Auxiliary.h"

using namespace FCUP;

PortalCommunicationPrx portal;
std::string IceStormInterfaceName = "StreamNotifications";

char **command_name_completion(const char *, int, int);
char *command_name_generator(const char *, int);

char *command_names[] = {
        (char*) "stream list",
        (char*) "stream search",
        (char*) "stream play",
        (char*) "exit",
        NULL
};

void playStream(std::string name){
    printf("ENTRA NO PLAYSTREAM\n");

    int pid = vfork();
    if ( pid < 0 ) {
        perror("fork failed");
        exit(1);
    }

    if ( pid == 0 ) {

        StreamsMap streamList = portal->sendStreamServersList();
        auto elem = streamList.find(name);
        printf("ALLAAAAH AKBAR\n");
        std::cout << "-------->" << elem->second.name << std::endl;


        if(elem != streamList.end()) {
            char **strings = NULL;
            size_t strings_size = 0;
            AddString(&strings, &strings_size, "ffplay");

            std::string hostname = elem->second.endpoint.ip;
            std::string port = elem->second.endpoint.port;
            std::string transport = elem->second.endpoint.transport;
            std::stringstream ss;
            ss << transport << "://" << hostname << ":" << port;
            const std::string &tmp = ss.str();
            const char *cstr = tmp.c_str();

            AddString(&strings, &strings_size, cstr);
            AddString(&strings, &strings_size, NULL);

            for (int i = 0; strings[i] != NULL; ++i) {
                printf("|%s|\n", strings[i]);
            }

            int fd = open("/home/tiaghoul/lixooo", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);

            execvp(strings[0], strings);
        }

    } else {
        printf("ffplay running...\n");
    }
}

void getStreamsList(){

    StreamsMap streamList = portal->sendStreamServersList();
    int size = (int) streamList.size();
    if(size > 0) {
        std::cout << size << " streams available:" << std::endl;
        int counter = 1;
        for (auto const& stream : streamList) {
            std::cout << "\t" << counter << ". " << stream.first << " Video Size: " << stream.second.videoSize << " Bit Rate: " << stream.second.bitrate << std::endl;
            counter++;
        }
        std::cout << std::endl << "-------------------" << std::endl;
    }
    else{
        std::cout << "No available streams atm." << std::endl;
    }
}

void searchKeyword(std::string keyword) {

}

class Subscriber : public Ice::Application {
public:

    virtual int run(int, char*[]);
};

class StreamNotificationsI : virtual public StreamNotifications{
public:
    virtual void reportAddition(const FCUP::StreamServerEntry& sse, const Ice::Current& ){
        std::cout << "STREAM COMECOU-> " << sse.name << std::endl;
    }
    virtual void reportRemoval(const FCUP::StreamServerEntry& sse, const Ice::Current&){
        std::cout << "STREAM ACABOU-> " << sse.name << std::endl;
    }
};


int Subscriber::run(int argc, char* argv[]) {

    int status = 0;
    IceStorm::TopicPrx topic;

    try {

        IceStorm::TopicManagerPrx manager = IceStorm::TopicManagerPrx::checkedCast(
                communicator()->propertyToProxy("TopicManager.Proxy"));
        if(!manager)
        {
            std::cerr << appName() << ": invalid proxy" << std::endl;
            return EXIT_FAILURE;
        }

        try
        {
            topic = manager->retrieve(IceStormInterfaceName);
        }
        catch(const IceStorm::NoSuchTopic&)
        {
            try
            {
                topic = manager->create(IceStormInterfaceName);
            }
            catch(const IceStorm::TopicExists&)
            {
                std::cerr << appName() << ": temporary failure. try again." << std::endl;
                return EXIT_FAILURE;
            }
        }

        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("StreamNotifications.Subscriber");


        Ice::Identity subId;
        subId.name = IceUtil::generateUUID();

        Ice::ObjectPrx subscriber = adapter->add(new StreamNotificationsI, subId);

        adapter->activate();
        IceStorm::QoS qos;

        subscriber = subscriber->ice_oneway();

        try
        {
            topic->subscribeAndGetPublisher(qos, subscriber);
        }
        catch(const IceStorm::AlreadySubscribed&)
        {

        }

        shutdownOnInterrupt();


//        IceStorm::TopicManagerPrx manager = IceStorm::TopicManagerPrx::checkedCast(
//
//                streamNotifier = StreamNotificationsPrx::uncheckedCast(obj);

        Ice::ObjectPrx base = communicator()->stringToProxy("Portal:default -p 9999");
        portal = PortalCommunicationPrx::checkedCast(base);
        if (!portal){
            throw "Invalid proxy";
        }

//        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter(IceStormInterfaceName);
//        adapter->add(, Ice::stringToIdentity(IceStormInterfaceName));
//        adapter->activate();


        rl_attempted_completion_function = command_name_completion;

        while(true){
            char *input;
            input = readline("-> ");
            printf("|%s|\n",input );
            add_history(input);

            std::string str(input);

            std::vector<std::string> userCommands = split(input, ' ');

            if (userCommands.empty()){
                continue;
            }

            if(userCommands[0] == "stream") {

                if (userCommands[1] == "list") {
                    getStreamsList();
                } else if (userCommands[1] == "play") {
                    printf("ENTRA NO PLAY\n");
                    playStream(userCommands[2]);
                } else if (userCommands[1] == "search") {
                    if(userCommands.size()>2) {
                        searchKeyword(userCommands[2]);
                    } else {
                        std::cout << "You need to pass one or more keywords.." << std::endl;
                    }
                } else{
                    //por os comandos disponiveis
                    std::cout << "Can't find that command" << std::endl;
                }
            }
            else if (userCommands[0] == "exit") {
                topic->unsubscribe(subscriber);
                return 0;
            }
            else{
                //por os comandos disponiveis
                std::cout << "Can't find that command" << std::endl;
            }
            free(input);
        }

    } catch (const Ice::Exception& ex) {
        std::cerr << ex << std::endl;
        status = 1;
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        status = 1;
    }

    if (communicator()){
        communicator()->destroy();
    }

    communicator()->waitForShutdown();

    return status;
}

int main(int argc, char* argv[])
{
    Subscriber app;
    app.main(argc,argv, "config.sub");
}

char **command_name_completion(const char *text, int start, int end)
{
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_name_generator);
}

char *command_name_generator(const char *text, int state)
{
    static int list_index, len;
    char *name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = command_names[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}
