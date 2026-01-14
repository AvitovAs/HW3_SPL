package bgu.spl.net.srv;

import java.util.ArrayList;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class ConcurrentArrayList<T> {
    private ArrayList<T> list;
    private final ReadWriteLock lock;

    public ConcurrentArrayList() {
        this.list = new ArrayList<>();
        this.lock = new ReentrantReadWriteLock();
    }

    public void add(T item) {
        lock.writeLock().lock();
        try {
            list.add(item);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public void remove(T item) {
        lock.writeLock().lock();
        try {
            list.remove(item);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public ArrayList<T> getSnapshot() {
        lock.readLock().lock();
        try {
            return new ArrayList<>(list);
        } finally {
            lock.readLock().unlock();
        }
    }

    public int size() {
        lock.readLock().lock();
        try {
            return list.size();
        } finally {
            lock.readLock().unlock();
        }
    }

    public boolean contains(T item) {
        lock.readLock().lock();
        try {
            return list.contains(item);
        } finally {
            lock.readLock().unlock();
        }
    }

    public boolean isEmpty() {
        lock.readLock().lock();
        try {
            return list.isEmpty();
        } finally {
            lock.readLock().unlock();
        }
    }
}
