#include <memory>
#include <utility>

#include "examples/peerconnection/peer_test/peerconnectionimpl.h"

#include <gtk/gtk.h>
#include "examples/peerconnection/client/linux/main_wnd.h"
#include "rtc_base/ssladapter.h"
#include "rtc_base/thread.h"

class CustomSocketServer : public rtc::PhysicalSocketServer {
 public:
  explicit CustomSocketServer(GtkMainWnd* wnd) : wnd_(wnd){}
  virtual ~CustomSocketServer() {}

  void SetMessageQueue(rtc::MessageQueue* queue) override {
    message_queue_ = queue;
  }

  bool Wait(int cms, bool process_io) override {
   while (gtk_events_pending())
      gtk_main_iteration();

    if (!wnd_->IsWindow()) {
      message_queue_->Quit();
    }

    return rtc::PhysicalSocketServer::Wait(0 /*cms == -1 ? 1 : cms*/,
                                           process_io);
  }

 protected:
  rtc::MessageQueue* message_queue_;
  GtkMainWnd* wnd_;
};

int main(int argc, char **argv)
{
  gtk_init(&argc, &argv);
#if !GLIB_CHECK_VERSION(2, 35, 0)
  g_type_init();
#endif
#if !GLIB_CHECK_VERSION(2, 31, 0)
  g_thread_init(NULL);
#endif

  GtkMainWnd wnd("server", 8888, false, false);
  wnd.Create();

  //use this socket server as the event sequence.
  //it will kill the process running.
  CustomSocketServer socket_server(&wnd);
  rtc::AutoSocketServerThread thread(&socket_server);

  rtc::scoped_refptr<PeerConnectionImpl> local_peerconnection(
      new rtc::RefCountedObject<PeerConnectionImpl>(&wnd));
  rtc::scoped_refptr<PeerConnectionImpl> remote_peerconnection(
      new rtc::RefCountedObject<PeerConnectionImpl>(&wnd));
 
  if(!local_peerconnection->InitPeerConnection() || !remote_peerconnection->InitPeerConnection())
  {
    return 0;
  }

  local_peerconnection->SetRemotePeerConnection(remote_peerconnection);
  remote_peerconnection->SetRemotePeerConnection(local_peerconnection);
  local_peerconnection->AddTracks();
  local_peerconnection->CreateOffer();
  thread.Run();
  wnd.Destroy();
  return 0;
}