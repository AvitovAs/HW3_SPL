package bgu.spl.net.srv;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

public class ConnectionImpl<T> implements Connections<T> {
    private ConcurrentHashMap<Integer, ConnectionHandler<T>> conconnections;
    private ConcurrentHashMap<String, CopyOnWriteArrayList<Integer>> channels;
    private ConcurrentHashMap<Integer, CopyOnWriteArrayList<String>> userChannels;
    private ConcurrentHashMap<Integer, ConcurrentHashMap<String, Integer>> userSubscriptions;

    public ConnectionImpl (){
        conconnections = new ConcurrentHashMap<Integer, ConnectionHandler<T>>();
        channels = new ConcurrentHashMap<String, CopyOnWriteArrayList<Integer>>();
        userChannels = new ConcurrentHashMap<Integer, CopyOnWriteArrayList<String>>();
        userSubscriptions = new ConcurrentHashMap<Integer, ConcurrentHashMap<String, Integer>>();
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
        conconnections.remove(connectionId);
        
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
        
        userSubscriptions.remove(connectionId);
    }

    @Override
    public void connect(int connectionId, ConnectionHandler<T> handler) {
        conconnections.putIfAbsent(connectionId, handler);
    }

    @Override
    public void subscribe(int connectionId, String channel, int subscriptionId) {
        channels.computeIfAbsent(channel, (k) -> new CopyOnWriteArrayList<Integer>()).add(connectionId);
        userChannels.computeIfAbsent(connectionId, (k) -> new CopyOnWriteArrayList<String>()).add(channel);
        userSubscriptions.computeIfAbsent(connectionId, (k) -> new ConcurrentHashMap<String, Integer>()).put(channel, subscriptionId);
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
        
        userSubscriptions.computeIfPresent(connectionId, (k, subscriptions) -> 
        { 
            subscriptions.remove(channel); 
            if (subscriptions.isEmpty()) 
                return null; 
            else 
                return subscriptions;
        });
    }

    @Override
    public int getSubscriptionId(int connectionId, String channel) {
        return userSubscriptions.getOrDefault(connectionId, new ConcurrentHashMap<>()).getOrDefault(channel, -1);
    }
}