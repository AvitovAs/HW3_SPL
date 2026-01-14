package bgu.spl.net.srv;

import java.util.ArrayList;

public class Channel {
    private String name;
    private ConcurrentArrayList<Integer> members;

    public Channel(String name) {
        this.name = name;
        this.members = new ConcurrentArrayList<>();
    }

    public synchronized void subscribe(Integer connectionId) {
        if (!members.contains(connectionId))
            members.add(connectionId);
    }

    public void unsubscribe(Integer connectionId) {
        members.remove(connectionId); 
    }

    public ArrayList<Integer> getMembers() {
        return members.getSnapshot();
    }

    public String getName() {
        return name;
    }

    public int getMembersCount() {
        return members.size();
    }
}