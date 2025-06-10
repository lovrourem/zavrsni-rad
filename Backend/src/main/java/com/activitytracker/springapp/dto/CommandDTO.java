package com.activitytracker.springapp.dto;

public class CommandDTO {
    private String mode;
    private String command;
    private String result;
    private Integer steps;
    private String sleep;

    public String getMode() {
        return mode;
    }

    public void setMode(String mode) {
        this.mode = mode;
    }

    public String getCommand() {
        return command;
    }

    public void setCommand(String command) {
        this.command = command;
    }

    public String getResult() {
        return result;
    }

    public void setResult(String result) {
        this.result = result;
    }

    public Integer getSteps(){
        return steps;
    }

    public void setSteps(Integer steps){
        this.steps = steps;
    }

    public String getSleep(){
        return sleep;
    }

    public void setSleep(String sleep){
        this.sleep = sleep;
    }
}
