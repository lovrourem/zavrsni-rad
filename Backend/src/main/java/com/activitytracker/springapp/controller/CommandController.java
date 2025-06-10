package com.activitytracker.springapp.controller;

import com.activitytracker.springapp.dto.CommandDTO;
import com.activitytracker.springapp.service.CommandService;

import java.util.HashMap;
import java.util.Map;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;



@RestController
@RequestMapping("/")
public class CommandController {

    private final CommandService commandService;

    public CommandController(CommandService commandService) {
        this.commandService = commandService;
    }

    // ===== MODE =====

    @PostMapping("/mode")
    public ResponseEntity<Void> setMode(@RequestBody CommandDTO dto) {
        commandService.setMode(dto.getMode());
        return ResponseEntity.ok().build();
    }

    @GetMapping("/mode")
    public ResponseEntity<CommandDTO> getMode() {
        CommandDTO dto = new CommandDTO();
        dto.setMode(commandService.getMode());
        dto.setCommand(commandService.getCommand());
        return ResponseEntity.ok(dto);
    }

    @PostMapping("/sleep")
    public ResponseEntity<Void> setSleep(@RequestBody CommandDTO dto) {
        commandService.setSleep(dto.getSleep());
        return ResponseEntity.ok().build();
    }

    @GetMapping("/sleep")
    public ResponseEntity<CommandDTO> getSleep() {
        CommandDTO dto = new CommandDTO();
        dto.setSleep(commandService.getSleep());
        return ResponseEntity.ok(dto);
    }

    // ===== COMMAND =====

    @PostMapping("/command")
    public ResponseEntity<Void> setCommand(@RequestBody CommandDTO dto) {
        commandService.setCommand(dto.getCommand());
        return ResponseEntity.ok().build();
    }

    @GetMapping("/command")
    public ResponseEntity<String> getCommand() {
        String command = commandService.getCommand();
        return command != null ? ResponseEntity.ok(command) : ResponseEntity.noContent().build();
    }

    // ===== RESULT =====

    @PostMapping("/result")
    public ResponseEntity<Void> setResult(@RequestBody CommandDTO dto) {
        commandService.setResult(dto.getResult());
        return ResponseEntity.ok().build();
    }

    @GetMapping("/result")
    public ResponseEntity<String> getResult() {
        String result = commandService.getResult();
        return result != null ? ResponseEntity.ok(result) : ResponseEntity.noContent().build();
    }

    @PostMapping("/steps")
    public ResponseEntity<Void> updateSteps(@RequestBody CommandDTO dto) {
        if(dto.getSteps() != null){
            commandService.addSteps(dto.getSteps());
        }
        return ResponseEntity.ok().build();
    }

    @GetMapping("/steps")
    public ResponseEntity<Integer> getSteps() {
        return ResponseEntity.ok(commandService.getSteps());
    }
    
    @GetMapping("/activity")
    public ResponseEntity<Map<String, Object>> getActivityMetrics() {
        int steps = commandService.getSteps();

        double kcal = steps * 0.05;
        double km = steps * 0.0008;

        Map<String, Object> result = new HashMap<>();
        result.put("steps", steps);
        result.put("kcal", Math.round(kcal * 10.0) / 10.0); 
        result.put("km", Math.round(km * 100.0) / 100.0);

        return ResponseEntity.ok(result);
    }
    

    @GetMapping("/status")
    public ResponseEntity<String> getDeviceStatus() {
        boolean connected = commandService.isDeviceConnected();
        return ResponseEntity.ok(connected ? "connected" : "disconnected");
    }

    @PostMapping("/checkin")
    public ResponseEntity<Void> checkin() {
        commandService.updateCheckin();
        return ResponseEntity.ok().build();
    }

}
