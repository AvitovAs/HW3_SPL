package bgu.spl.net.srv;
import java.util.concurrent.ConcurrentHashMap;

public class ConnectionImpl<T> implements Connections<T> {
    private ConcurrentHashMap<Integer, ConnectionHandler<T>> conconnections;
    private ConcurrentHashMap<String, Channel> channels;
    private ConcurrentHashMap<Integer, ConcurrentArrayList<String>> userChannels;

    public ConnectionImpl (){
        conconnections = new ConcurrentHashMap<Integer, ConnectionHandler<T>>();
        channels = new ConcurrentHashMap<String, Channel>();
        userChannels = new ConcurrentHashMap<Integer, ConcurrentArrayList<String>>();
    }

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = conconnections.get(connectionId);
        if (handler != null) {
            handler.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg) {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'send'");

    }

    @Override
    public void disconnect(int connectionId) {
        conconnections.remove(connectionId);

        ConcurrentArrayList<String> userChs = userChannels.get(connectionId);
        if (userChs != null) {
            synchronized (userChs) {
                for (String channelName : userChs.getSnapshot()) {
                    Channel ch = channels.get(channelName);
                    if (ch != null) 
                        ch.unsubscribe(connectionId);
                    userChannels.remove(connectionId);
                }
            }
        }
    }

    @Override
    public void connect(int connectionId, ConnectionHandler<T> handler) {
        conconnections.putIfAbsent(connectionId, handler);
        userChannels.putIfAbsent(connectionId, new ConcurrentArrayList<String>());
    }

    @Override
    public void subscribe(int connectionId, String channel) {
        channels.putIfAbsent(channel, new Channel(channel));
        Channel ch = channels.get(channel);

        ch.subscribe(connectionId);
        userChannels.get(connectionId).add(channel);
    }

    @Override
    public void unsubscribe(int connectionId, String channel) {
        Channel ch = channels.get(channel);
        if (ch != null)
            synchronized (ch) {
                ch.unsubscribe(connectionId);
                userChannels.get(connectionId).remove(channel);
                if (ch.getMembersCount() == 0)
                    channels.remove(channel);
            }
    }
}
