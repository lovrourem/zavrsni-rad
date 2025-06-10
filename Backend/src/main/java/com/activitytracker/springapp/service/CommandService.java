package com.activitytracker.springapp.service;

import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;

import java.time.Duration;
import java.time.Instant;
import java.util.concurrent.atomic.AtomicInteger;

@Service
public class CommandService {

    private String currentMode;
    private String currentCommand;
    private String currentResult;
    private Instant lastCheckinTime;
    private String sleepState;
    private final AtomicInteger stepCounter = new AtomicInteger(0);


    public void setMode(String mode) {
        this.currentMode = mode;
    }

    public String getMode() {
        return this.currentMode;
    }

    public void setCommand(String command) {
        this.currentCommand = command;
    }

    public String getCommand() {
        return this.currentCommand;
    }

    public void setResult(String result) {
        this.currentResult = result;
    }

    public String getResult() {
        return this.currentResult;
    }

    public void addSteps(int steps){
        stepCounter.addAndGet(steps);
        System.out.println(stepCounter);
    }

    public int getSteps(){
        return stepCounter.get();
    }

    @Scheduled(cron = "0 0 0 * * *")
    public void resetSteps(){
        stepCounter.set(0);
    }

    public String getSleep(){
        return sleepState;
    }

    public void setSleep(String sleep){
        this.sleepState = sleep;
    }

    public boolean isDeviceConnected() {
        return lastCheckinTime != null &&
               Duration.between(lastCheckinTime, Instant.now()).toSeconds() < 5;
    }

    public void updateCheckin() {
        this.lastCheckinTime = Instant.now();
        System.err.println("Device checked in at: " + lastCheckinTime);
    }
}

