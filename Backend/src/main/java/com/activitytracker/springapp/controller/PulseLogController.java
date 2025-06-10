package com.activitytracker.springapp.controller;
import com.activitytracker.springapp.dto.PulseLogDTO;
import com.activitytracker.springapp.service.PulseLogService;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/pulselogs")
public class PulseLogController {

    private final PulseLogService pulseLogService;

    public PulseLogController(PulseLogService pulseLogService) {
        this.pulseLogService = pulseLogService;
    }

    @PostMapping
    public ResponseEntity<String> receivePulseLogs(@RequestBody List<PulseLogDTO> pulseLogs) {
        pulseLogService.processLogs(pulseLogs);
        return ResponseEntity.ok("Logs received");
    }

    @PostMapping("/movement")
    public ResponseEntity<String> receiveMovementCount(@RequestBody Map<String, Integer> body) {
        if (body.containsKey("movementCount")) {
            pulseLogService.saveMovementCount(body.get("movementCount"));
            return ResponseEntity.ok("Movement count saved");
        } else {
            return ResponseEntity.badRequest().body("Missing 'movementCount'");
        }
    }

    @PostMapping("/reset")
    public ResponseEntity<String> resetLogsAndMovement() {
        pulseLogService.resetAll();
        return ResponseEntity.ok("Data reset");
    }

    @GetMapping("/deepsleep")
    public ResponseEntity<Object> getDeepSleepSummary() {
        try {
            String result = pulseLogService.analyzeDeepSleep();
            String[] parts = result.split(";");
            double deep = Double.parseDouble(parts[0]);
            double total = Double.parseDouble(parts[1]);
            double percent = Double.parseDouble(parts[2]);
            int movement = Integer.parseInt(parts[3]);

            Map<String, Object> response = new HashMap<>();
            response.put("deepHours", deep);
            response.put("totalHours", total);
            response.put("percentage", percent);
            response.put("movementCount", movement);

            System.out.println("SLEEP SUMMARY: " + response);
            return ResponseEntity.ok(response);
        } catch (Exception e) {
            System.err.println("Failed to parse deep sleep summary: " + e.getMessage());
            return ResponseEntity.internalServerError().body("Error: " + e.getMessage());
        }
    }
}
