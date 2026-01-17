package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.out.println("Usage: java StompServer <port>");
            return;
        }

        int port = Integer.parseInt(args[0]);

        // Thread-per-client server
        Server.threadPerClient(
                port,
                StompMessagingProtocolImpl::new,
                StompEncoderDecoderImpl::new
        ).serve();

        // OR Reactor pattern:
        // Server.reactor(
        //         Runtime.getRuntime().availableProcessors(),
        //         port,
        //         StompMessagingProtocolImpl::new,
        //         StompEncoderDecoderImpl::new
        // ).serve();
    }
}
