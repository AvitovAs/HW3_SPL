package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: java StompServer <port> <mode>");
            return;
        }
        
        String mode = args[1];
        int port = Integer.parseInt(args[0]);

        if (mode.equals("TPC")) {           
            Server.threadPerClient(
                port,
                StompMessagingProtocolImpl::new,
                StompEncoderDecoderImpl::new
            ).serve();
        }
        else if (mode.equals("REACTOR")) {
            Server.reactor(
                Runtime.getRuntime().availableProcessors(),
                port,
                StompMessagingProtocolImpl::new,
                StompEncoderDecoderImpl::new
            ).serve();
        }
        else{
            System.out.println("Invalid mode. Use 'TPC' or 'REACTOR'.");
        }
    }
}
