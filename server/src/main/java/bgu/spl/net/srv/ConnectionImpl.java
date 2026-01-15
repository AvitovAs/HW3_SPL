package bgu.spl.net.srv;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

public class ConnectionImpl<T> implements Connections<T> {
    private ConcurrentHashMap<Integer, ConnectionHandler<T>> conconnections;
    private ConcurrentHashMap<String, CopyOnWriteArrayList<Integer>> channels;
    private ConcurrentHashMap<Integer, CopyOnWriteArrayList<String>> userChannels;

    public ConnectionImpl (){
        conconnections = new ConcurrentHashMap<Integer, ConnectionHandler<T>>();
        channels = new ConcurrentHashMap<String, CopyOnWriteArrayList<Integer>>();
        userChannels = new ConcurrentHashMap<Integer, CopyOnWriteArrayList<String>>();
    }

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = conconnections.get(connectionId);
        if (handler != null){
            handler.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg) {
        channels.computeIfPresent(channel, (k, subs) -> 
        { 
            for (Integer connectionId : subs){
                send(connectionId, msg);
            }
            return subs;
        });
    }

    @Override
    public void disconnect(int connectionId) {
        userChannels.computeIfPresent(connectionId, (k, channelList) -> 
        { 
            for (String channel : channelList){
                channels.computeIfPresent(channel, (ch, subs) -> 
                { 
                    subs.remove(Integer.valueOf(connectionId)); 
                    if (subs.isEmpty()) 
                        return null; 
                    else 
                        return subs;
                });
            }
            return null;
        });
    }

    @Override
    public void connect(int connectionId, ConnectionHandler<T> handler) {
        conconnections.putIfAbsent(connectionId, handler);
    }

    @Override
    public void subscribe(int connectionId, String channel) {
        channels.computeIfAbsent(channel, (k) -> new CopyOnWriteArrayList<Integer>()).add(connectionId);
        userChannels.computeIfAbsent(connectionId, (k) -> new CopyOnWriteArrayList<String>()).add(channel);
    }

    @Override
    public void unsubscribe(int connectionId, String channel) {
        channels.computeIfPresent(channel, (k, subs) -> 
        { 
            subs.remove(Integer.valueOf(connectionId)); 
            if (subs.isEmpty()) 
                return null; 
            else 
                return subs;
        });

        userChannels.computeIfPresent(connectionId, (k, channels) -> 
        { 
            channels.remove(channel); 
            if (channels.isEmpty()) 
                return null; 
            else 
                return channels;
        });
        
    }
}
