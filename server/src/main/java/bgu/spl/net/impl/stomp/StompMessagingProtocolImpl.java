package bgu.spl.net.impl.stomp;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {
    private volatile static AtomicLong messageIdCounter = new AtomicLong(0);
    private boolean shouldTerminate;
    private int connectionId;
    private Connections<String> connections;
    private boolean isConnected;
    private Map<String, String> subscriptionMap = new HashMap<>();

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
        if (packet == null) {
            handleError(packet, "Malformed STOMP frame");
            return;
        }

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
        String receipt = packet.headers.get("receipt");
        if (receipt == null)
            handleError(packet, "Missing receipt header for DISCONNECT");
        
        shouldTerminate = true;
        isConnected = false;
        connections.disconnect(connectionId);
        subscriptionMap.clear();
        sendReceipt(receipt);
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
        subscriptionMap.put(id, destination);

        String receipt = packet.headers.get("receipt");
        if (receipt != null)
            sendReceipt(receipt);
    }

    private void handleUnsubscribe(StompPacket packet) {
        String id = packet.headers.get("id");
        if (id == null) {
            handleError(packet, "Missing required headers for UNSUBSCRIBE");
            return;
        }

        String destination = subscriptionMap.get(id);
        if (destination == null) {
            handleError(packet, "No subscription found for id: " + id);
            return;
        }

        connections.unsubscribe(connectionId, destination);
        subscriptionMap.remove(id);

        String receipt = packet.headers.get("receipt");
        if (receipt != null)
            sendReceipt(receipt);
    }

    private void handleSend(StompPacket packet) {
        String destination = packet.headers.get("destination");
        if (destination == null) {
            handleError(packet, "Missing destination header for SEND");
            return;
        }

        String body = packet.body;
        for (Integer subscriberId : connections.getChannelSubscribers(destination)) {
            int subscriptionId = connections.getSubscriptionId(subscriberId, destination);
            String response = "MESSAGE\nsubscription:" + subscriptionId + "\nmessage-id:" + messageIdCounter + "\ndestination:" + destination + "\n\n" + body + "\u0000";
            connections.send(subscriberId, response);
        }

        messageIdCounter.incrementAndGet();
    }

    private void handleError(StompPacket packet, String errorMessage) {
        String receipt = packet.headers.get("receipt");
        String response = "ERROR\nmessage:" + errorMessage + "\n";
        if (receipt != null) 
            response += "receipt-id:" + receipt + "\n";

        response += "\n" + packet.body + "\n\u0000";
        connections.send(connectionId, response);
        shouldTerminate = true;
        isConnected = false;
        subscriptionMap.clear();
        connections.disconnect(connectionId);
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

    private void sendReceipt(String receiptId) {
        String response = "RECEIPT\nreceipt-id:" + receiptId + "\n\n\u0000";
        connections.send(connectionId, response);
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
