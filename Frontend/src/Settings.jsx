import styles from './Settings.module.css';
import { useState, useEffect } from 'react';
import { Link } from "react-router-dom";
import button from '../public/backbutton.png';

function Settings() {
    const [activeMode, setActiveMode] = useState(null); 
    const [message, setMessage] = useState("");

    const fetchMode = async () => {
        try {
            const response = await fetch("http://192.168.0.212:8080/mode");
            if (!response.ok) throw new Error("Failed to fetch current mode");
            const data = await response.json();
            setActiveMode(data.mode); 
        } catch (error) {
            console.error("Error fetching mode:", error);
            setMessage("Unable to fetcah current mode.");
        }
    };
    

    const checkDeviceStatus = async () => {
        try {
            const res = await fetch("http://192.168.0.212:8080/status");
            if (!res.ok) throw new Error("Status check failed");
            const status = await res.text();
            return status === "connected";
        } catch (error) {
            console.error("Device status check error:", error);
            return false;
        }
    };

    const sendMode = async (mode) => {
        const isConnected = await checkDeviceStatus();

        if (!isConnected) {
            setMessage("Device not connected. Please press the button on your device.");
            return;
        }

        try {
            const response = await fetch("http://192.168.0.212:8080/mode", {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({ mode })
            });

            if (!response.ok) throw new Error("Failed to send mode");

            setActiveMode(mode);
            setMessage("Mode updated successfully.");
        } catch (error) {
            console.error("Error sending mode:", error);
            setMessage("Error while sending mode to device.");
        }
    };

    useEffect(() => {
        fetchMode();
    }, []);

    return (
        <div className={styles.settingsContainer}>
            <div className={styles.header}>
                <Link to="/" className={styles.back}>
                    <img src={button} className={styles.backbutton} />
                </Link>
            </div>

            <div className={styles.card}>
                <h3 className={styles.cardTitle}>Working mode</h3>
                <div className={styles.buttonGroup}>
                    {["Full", "Half", "Low"].map(mode => (
                        <button
                            key={mode}
                            className={`${styles.button} ${activeMode === mode ? styles.active : ""}`}
                            onClick={() => sendMode(mode)}
                        >
                            {mode}
                        </button>
                    ))}
                </div>
            </div>

            {/*<div className={styles.card}> 
                <h3 className={styles.cardTitle}>Device</h3>
                <div className={styles.buttonGroup}>
                    <button className={styles.button}>Button</button>
                </div>
            </div> */}

            {message && <p style={{ color: 'white', textAlign: 'center' }}>{message}</p>}
        </div>
    );
}

export default Settings;
