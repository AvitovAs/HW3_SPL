package bgu.spl.net.impl.stomp;

import java.util.concurrent.ConcurrentHashMap;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {
    private boolean shouldTerminate;
    private int connectionId;
    private Connections<String> connections;
    private boolean isConnected;

    public StompMessagingProtocolImpl() {
        this.shouldTerminate = false;
        this.isConnected = false;
    }

    @Override
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(String message) {
        StompPacket packet = parseMessage(message);

        if (!isConnected && !packet.command.equals("CONNECT")) {
            handleError(packet, "Not connected" );
        }

        switch (packet.command) {
            case "DISCONNECT":
                handleDisconnect(packet);
                break;
            case "CONNECT":
                handleConnect(packet);
                break;
            case "SUBSCRIBE":
                handleSubscribe(packet);
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(packet);
                break;
            case "SEND":
                handleSend(packet);
                break;
            default:
                handleError(packet, "Unknown command");
                break;
        }        
    }

    private void handleConnect(StompPacket packet) {
        if (isConnected) {
            handleError(packet, "Already connected");
            return;
        }
        
        String acceptVersion = packet.headers.get("accept-version");
        String host = packet.headers.get("host");
        String login = packet.headers.get("login");
        String passcode = packet.headers.get("passcode");
        if (acceptVersion == null || host == null || login == null || passcode == null) {
            handleError(packet, "Missing required headers");
            return;
        }

        // TODO: Validate login and passcode

        isConnected = true;
        String response = "CONNECTED\nversion:1.2\n\n\u0000";
        connections.send(connectionId, response);
    }

    private void handleDisconnect(StompPacket packet) {
        
    }

    private void handleSubscribe(StompPacket packet) {
        String destination = packet.headers.get("destination");
        String id = packet.headers.get("id");
        if (destination == null || id == null) {
            handleError(packet, "Missing required headers for SUBSCRIBE");
            return;
        }

        int subscriptionId;
        try {
            subscriptionId = Integer.parseInt(id);
        } catch (NumberFormatException e) {
            handleError(packet, "Invalid subscription id");
            return;
        }

        connections.subscribe(connectionId, destination, subscriptionId);
    }

    private void handleUnsubscribe(StompPacket packet) {
        
    }

    private void handleSend(StompPacket packet) {
        
    }

    private void handleError(StompPacket packet, String errorMessage) {
        
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private static class StompPacket {
        String command;
        ConcurrentHashMap<String, String> headers;
        String body;
    }

    private StompPacket parseMessage(String message) {
        StompPacket stompPacket = new StompPacket();
        String[] parts = message.split("\n\n", 2);

        if (parts.length == 0)
            return null;

        String headerPart = parts[0];
        stompPacket.body = parts.length > 1 ? parts[1] : "";

        String[] lines = headerPart.split("\n");
        if (lines.length == 0)
            return null;

        stompPacket.command = lines[0];
        stompPacket.headers = new ConcurrentHashMap<>();

        for (int i = 1; i < lines.length; i++) {
            String[] headerParts = lines[i].split(":", 2);
            if (headerParts.length == 2) {
                stompPacket.headers.put(headerParts[0], headerParts[1]);
            }
        }

        return stompPacket;
    }

}
