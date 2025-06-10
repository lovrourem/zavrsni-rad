package com.activitytracker.springapp.dto;

public class PulseLogDTO {
    private long timestamp;
    private int avgBpm;

    public long getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(long timestamp) {
        this.timestamp = timestamp;
    }

    public int getAvgBpm() {
        return avgBpm;
    }

    public void setAvgBpm(int avgBpm) {
        this.avgBpm = avgBpm;
    }

}

