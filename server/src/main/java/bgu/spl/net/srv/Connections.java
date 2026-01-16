package bgu.spl.net.srv;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void connect(int connectionId, ConnectionHandler<T> handler);

    void disconnect(int connectionId);

    void subscribe(int connectionId, String channel, int subscriptionId);

    void unsubscribe(int connectionId, String channel);

    int getSubscriptionId(int connectionId, String channel);

    Integer[] getChannelSubscribers(String channel);
}
