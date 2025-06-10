package com.activitytracker.springapp.service;
import org.springframework.stereotype.Service;
import com.activitytracker.springapp.dto.PulseLogDTO;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

@Service
public class PulseLogService {
    private final List<PulseLogDTO> storedLogs = new ArrayList<>();
    private int movementCount = 0;

    public void processLogs(List<PulseLogDTO> logs) {
        for (PulseLogDTO log : logs) {
            System.out.printf("Processing pulse log: Timestamp=%d, Avg BPM=%d%n", log.getTimestamp(), log.getAvgBpm());
        }
        storedLogs.addAll(logs);
    }

    public void saveMovementCount(int count) {
        System.out.println("Movement count added: " + count + "total: " + this.movementCount);
        this.movementCount += count;
    }

    public void resetAll() {
        storedLogs.clear();
        movementCount = 0;
        System.out.println("All logs and movement count reset.");
    }

    public String analyzeDeepSleep() {
        storedLogs.sort((a, b) -> Long.compare(a.getTimestamp(), b.getTimestamp()));

        long total = 0;
        long deep = 0;

        for (int i = 0; i < storedLogs.size() - 1; i++) {
            PulseLogDTO current = storedLogs.get(i);
            PulseLogDTO next = storedLogs.get(i + 1);

            long interval = next.getTimestamp() - current.getTimestamp();
            total += interval;

            if (current.getAvgBpm() < 60) {
                deep += interval;
            }
        }
        double deepHours = deep / 3600000.0;
        double totalHours = total / 3600000.0;
        double percentage = total> 0 ? (deep * 100.0 / total) : 0;
        
        return String.format(Locale.US, "%.2f;%.2f;%.2f;%d", deepHours, totalHours, percentage, movementCount);    
    }
}


