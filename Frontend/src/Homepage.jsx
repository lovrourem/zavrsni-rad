import { useState, useEffect } from 'react'
import { Link } from "react-router-dom"

import styles from "./Homepage.module.css"
import settings from "./assets/settings.png"
import balloon from "../public/balloon.png"
import pulse from "../public/pulse.png"
import steps from "../public/steps.png"

const baseUrl = import.meta.env.VITE_API_BASE_URL;

function Homepage(){
    const [isOn, setIsOn] = useState(false);
    const [message, setMessage] = useState("")
    const [measurementType, setMeasurementType] = useState("");
    const [heartbeat, setHeartbeat] = useState(null);
    const [temperature, setTemperature] = useState(null);
    const [loading, setLoading] = useState(false);
    const [stepsCounter, setStepsCounter] = useState(0);
    const [kcal, setKcal] = useState(0);
    const [km, setKm] = useState(0);
    const [deepSleepInfo, setDeepSleepInfo] = useState({
        deepMinutes: 0,
        totalMinutes: 0,
        percentage: 0,
        movementCount: 0
    });

    const handleSleep = async (type) => {
        const isConnected = await checkDeviceStatus();
        if (!isConnected) {
            alert("Device is not connected. Please click the button");
            return;
        }
        try{
            if(type === "ON"){
                await fetch(`${baseUrl}/pulselogs/reset`, {
                    method: "POST"
                });
            }

            await fetch (`${baseUrl}/sleep`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({sleep: type})
            });
            setIsOn(type === "ON");

            await new Promise(resolve => setTimeout(resolve, 2000));
            if(type === "OFF"){
                await fetchDeepSleepInfo();
            }

        } catch (error){
            console.error("Failed to go to sleep mode:", error);
        }
    }

    const fetchDeepSleepInfo = async () => {
        try {
            const res = await fetch(`${baseUrl}/pulselogs/deepsleep`);
            if (res.ok) {
                const data = await res.json();
                setDeepSleepInfo(data);
                console.log("Deep sleep info:", data);
            }
        } catch (error) {
            console.error("Error fetching deep sleep summary:", error);
        }
    };

    const fetchActivityMetrics = async () => {
        try {
            const res = await fetch(`${baseUrl}/activity`);
            if (res.ok) {
                const data = await res.json();
                setKcal(data.kcal.toString());
                setKm(data.km.toString());
            }
        } catch (error) {
            console.error("Failed to fetch activity metrics:", error);
        }
    };

    const fetchSleepState = async () => {
        try{
            const res = await fetch(`${baseUrl}/sleep`);
            if (res.ok){
                const data = await res.json();
                setIsOn(data.sleep === "ON");
            }
        } catch(error) {
            console.error("Error fetching sleep state:", error);
        }
    }

    const fetchSteps = async () => {
        try {
            const response = await fetch(`${baseUrl}/steps`);
            if (!response.ok) throw new Error("Failed to fetch steps");
            const data = await response.json();
            setStepsCounter(data);
        } catch (error) {
            console.error("Error fetching steps:", error);
          }
    };

    const checkDeviceStatus = async () => {
        try {
            const res = await fetch(`${baseUrl}/status`);
            if (!res.ok) throw new Error("Status check failed");
            const status = await res.text();
            return status === "connected";
        } catch (error) {
            console.error("Device status check error:", error);
            return false;
        }
    };

    const handleHeartbeatClick = async () => {
        const isConnected = await checkDeviceStatus();
        if (!isConnected) {
            alert("Device is not connected. Please click the button");
            return;
        }
    
        try {
            await fetch(`${baseUrl}/result`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ result: "" }),
            });
    
            await fetch(`${baseUrl}/command`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                },
                body: JSON.stringify({ command: "measure_heartbeat" }),
            });
    
            setMessage("Heartbeat measurement triggered.");
            setLoading(true);
            startPollingForResult("heartbeat");
    
        } catch (error) {
            console.error("Failed to send heartbeat command:", error);
            setMessage("Error sending heartbeat command.");
        }
    };

    const handleTemperatureClick = async () => {
        const isConnected = await checkDeviceStatus();
        if (!isConnected) {
            alert("Device is not connected. Please click the button");
            return;
        }
    
        try {
            await fetch(`${baseUrl}/result`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ result: "" }),
            });
    
            await fetch(`${baseUrl}/command`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                },
                body: JSON.stringify({ command: "measure_temperature" }),
            });
            setLoading(true);
            startPollingForResult("temperature");
    
        } catch (error) {
            console.error("Failed to send temperature command:", error);
            setMessage("Error sending temperature command.");
        }
    };
    

    const startPollingForResult = (type) => {
        let attempts = 0;
        const maxAttempts = 10;
    
        const intervalId = setInterval(async () => {
            attempts++;
    
            try {
                const res = await fetch(`${baseUrl}/result`);
                if (res.ok) {
                    const result = await res.text();
                    if (result) {
                        setLoading(false);
                        clearInterval(intervalId);

                        if (type === "heartbeat") {
                            setHeartbeat(result);
                        } else if (type === "temperature") {
                            setTemperature(result);
                        }

                    }
                }
            } catch (error) {
                console.error("Polling error:", error);
            }
    
            if (attempts >= maxAttempts) {
                clearInterval(intervalId);
                setLoading(false);
                setMessage("Timeout: No result received.");
            }
        }, 5000);
    };

    useEffect(() => {
        fetchSteps();
        fetchSleepState();
        fetchDeepSleepInfo();
        const intervalId = setInterval(fetchSteps, 5000);

        return () => clearInterval(intervalId);
    }, []);

    useEffect(() => {
        fetchActivityMetrics();
    }, [stepsCounter]);

    
    

    return(
        <div>
            <div className={styles.header}>
                <Link to="/Settings" className={styles.settings}>
                    <img className={styles.settingsButton} src={settings}></img>
                </Link>
            </div>
            <div className={styles.display}>
                <div className={styles.container}>
                    <div className={styles.item}>
                        <div className={styles.bpm}
                            onClick={handleHeartbeatClick}>
                            <div className={styles.bpmTop}>
                                <div className={styles.bpmLeft}>
                                    <span className={styles.label}>Heart</span>
                                    <span className={styles.value}>{heartbeat}</span>
                                    <span className={styles.unit} >bpm</span>
                                </div>
                                <div className={styles.bpmRight}>
                                    <img className={styles.balloon} src={balloon}></img>
                                </div>
                            </div>
                            <div className={styles.bpmBottom}>
                                <img className={styles.pulse} src={pulse}></img>
                            </div>
                        </div>
                    </div>
                    <div className={styles.item}>
                        <div className={styles.bodyTemp} onClick={handleTemperatureClick}>
                            <div className={styles.tempHead}>Body temperature</div>
                            <div className={styles.tempValue}>{temperature} °C</div>
                        </div>
                    </div>
                    <div className={`${styles.item} ${styles.item1}`}>
                        <div className={styles.activity}>
                            <div className={styles.actHead}>Activity</div>
                            <div className={styles.actVal}>
                                <div className={styles.actLeft}>{stepsCounter}</div>
                                <div className={styles.actRight}>
                                    <img src={steps}></img>
                                </div>
                            </div>
                            <div className={styles.actFoot}>Steps</div>
                        </div>
                    </div>
                    <div className={`${styles.item} ${styles.item2}`}>
                        <div className={styles.sleepSummary}>
                            <div className={styles.sleepValues}>
                                <p>Deep Sleep</p>
                                <p>{deepSleepInfo.deepMinutes.toFixed(2)}h</p>
                            </div>
                            <div className={styles.sleepValues}>
                                <p>Total Sleep</p>
                                <p>{deepSleepInfo.totalMinutes.toFixed(2)}h</p>
                            </div>
                            <div className={styles.sleepValues}>
                                <p>Deep %</p>
                                <p>{deepSleepInfo.percentage.toFixed(2)}%</p>
                            </div>
                            <div className={styles.sleepValues}>
                                <p>Movements</p>
                                <p>{deepSleepInfo.movementCount}</p>
                            </div>
                        </div>
                    </div>
                    <div className={styles.item}>
                        <div className={styles.sleepButtons}>
                            <span>Sleeping</span>
                            <button
                                className={`${styles.onButton} ${isOn ? styles.activeOn : ''}`}
                                onClick={() => handleSleep("ON")}
                            >
                                ON
                            </button>
                            <button
                                className={`${styles.offButton} ${!isOn ? styles.activeOff : ''}`}
                                onClick={() => handleSleep("OFF")}
                            >
                                OFF
                            </button>
                        </div>
                    </div>
                    <div className={styles.item}>
                        <div className={styles.topCal}>
                            <div>{kcal}</div>
                            <div>Kcal</div>
                        </div>
                        <div className={styles.bottomKm}>
                            <div>{km}</div>
                            <div>KM</div>
                        </div>
                    </div>
                    {loading && (
                    <div className={styles.popupOverlay}>
                        <div className={styles.popup}>
                        <p>Measuring... please wait</p>
                        </div>
                    </div>
                    )}
                </div>
            </div>
        </div>
    )
}

export default Homepage