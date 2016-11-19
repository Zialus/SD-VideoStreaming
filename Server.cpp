#include <Ice/Ice.h>
#include <StreamServer.h>

using namespace std;
using namespace PortalServerCommunication;

int
main(int argc, char* argv[])
{
    int status = 0;
    Ice::CommunicatorPtr ic;
    try {
        ic = Ice::initialize(argc, argv);
        Ice::ObjectPrx base = ic->stringToProxy("Portal:default -p 10000");
        CommunicationPrx portal = CommunicationPrx::checkedCast(base);
        if (!portal)
            throw "Invalid proxy";

				StringSequence registrationInfo;
				registrationInfo.push_back("a");
				registrationInfo.push_back("b");
				registrationInfo.push_back("c");
				portal->registerStream(registrationInfo);
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
    }
    if (ic)
        ic->destroy();
    return status;
}
