import sys
import re
import csv
import serial
import threading
import queue
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QTableWidget, QTableWidgetItem,
                             QPushButton, QHBoxLayout)
from PyQt5.QtCore import QTimer

class CANMonitorApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CAN Monitor")
        self.setGeometry(100, 100, 1200, 700)

        # Haupt-Widget und Layout
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.layout = QVBoxLayout()
        self.central_widget.setLayout(self.layout)

        # Tabelle 1 für CAN-IDs und Daten
        self.can_id_table = QTableWidget()
        self.can_id_table.setColumnCount(10)  # Um alle Daten darzustellen
        self.can_id_table.setHorizontalHeaderLabels(
            ["ID", "Timestamp", "Length", "D0", "D1", "D2", "D3", "D4", "D5", "D6"]
        )

        # Anpassung der Spaltenbreiten
        self.can_id_table.setColumnWidth(0, 80)  # CAN ID
        self.can_id_table.setColumnWidth(1, 350)  # Timestamp (erweitert)
        self.can_id_table.setColumnWidth(2, 60)  # Length
        for i in range(3, 10):
            self.can_id_table.setColumnWidth(i, 50)  # D0 bis D6 reduziert

        self.layout.addWidget(self.can_id_table)

        # Tabelle 2 für spezielle Werte (Ignition, Turn Signals, Doors, Headlamps)
        self.extra_data_table = QTableWidget()
        self.extra_data_table.setColumnCount(2)
        self.extra_data_table.setHorizontalHeaderLabels(["Parameter", "Value"])
        self.layout.addWidget(self.extra_data_table)

        # Start-Button für die Überwachung
        self.start_button = QPushButton("Start CAN Monitor")
        self.start_button.clicked.connect(self.start_monitoring)
        self.layout.addWidget(self.start_button)

        # Serielle Verbindung und Threads
        self.serial_connection = None
        self.queue = queue.Queue()
        self.read_thread = None
        self.running = False

        # Timer für GUI-Updates
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_gui)

        # CSV-Datei für Datenlog
        self.csv_file = 'can_data_log.csv'
        self.initialize_csv()

    def initialize_csv(self):
        try:
            with open(self.csv_file, mode='x', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(["Timestamp", "CAN ID", "Length", "D0", "D1", "D2", "D3", "D4", "D5", "D6"])
        except FileExistsError:
            pass  # CSV-Datei existiert bereits

    def start_monitoring(self):
        try:
            # Serielle Verbindung zum CAN-Bus
            self.serial_connection = serial.Serial('COM6', 115200, timeout=0.1)
            self.running = True
            self.read_thread = threading.Thread(target=self.read_serial_data)
            self.read_thread.daemon = True
            self.read_thread.start()
            self.timer.start(10)  # Update alle 10ms
        except serial.SerialException as e:
            print(f"Fehler beim Starten der seriellen Verbindung: {e}")

    def read_serial_data(self):
        while self.running:
            try:
                line = self.serial_connection.readline().decode('utf-8', errors='ignore').strip()
                match = re.match(r'([0-9A-Fa-f]{3}); (\d); ((?:[0-9A-Fa-f]{2},\s?)*[0-9A-Fa-f]{2})', line)
                if match:
                    can_id = match.group(1)
                    length = int(match.group(2))
                    raw_data = match.group(3).strip().split(', ')
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    self.queue.put((can_id, raw_data, timestamp, length))
            except Exception as e:
                print(f"Fehler beim Lesen der seriellen Daten: {e}")

    def update_gui(self):
        while not self.queue.empty():
            can_id, raw_data, timestamp, length = self.queue.get()
            self.update_can_id_table(can_id, raw_data, timestamp, length)
            self.parse_can_data(can_id, raw_data, timestamp)

    def update_can_id_table(self, can_id, raw_data, timestamp, length):
        # Suche nach der CAN ID in der Tabelle und aktualisiere die vorhandene Zeile oder füge eine neue Zeile hinzu
        found_row = -1
        row_count = self.can_id_table.rowCount()
        for row in range(row_count):
            if self.can_id_table.item(row, 0).text() == can_id:
                found_row = row
                break

        if found_row == -1:
            # Neue Zeile hinzufügen, wenn die CAN ID noch nicht vorhanden ist
            row_count = self.can_id_table.rowCount()
            self.can_id_table.insertRow(row_count)
            self.can_id_table.setItem(row_count, 0, QTableWidgetItem(can_id))
            self.can_id_table.setItem(row_count, 1, QTableWidgetItem(timestamp))
            self.can_id_table.setItem(row_count, 2, QTableWidgetItem(str(length)))

            # Datenbytes D0 bis D6 (maximal 8 Bytes, falls weniger, werden leere Felder gesetzt)
            for i in range(min(length, 7)):
                self.can_id_table.setItem(row_count, i + 3, QTableWidgetItem(raw_data[i]))

            # Falls weniger als 7 Bytes, leere Zellen setzen
            for i in range(length, 7):
                self.can_id_table.setItem(row_count, i + 3, QTableWidgetItem(""))
        else:
            # Daten in der gefundenen Zeile aktualisieren
            self.can_id_table.setItem(found_row, 1, QTableWidgetItem(timestamp))
            self.can_id_table.setItem(found_row, 2, QTableWidgetItem(str(length)))

            # Datenbytes D0 bis D6 (maximal 8 Bytes, falls weniger, werden leere Felder gesetzt)
            for i in range(min(length, 7)):
                self.can_id_table.setItem(found_row, i + 3, QTableWidgetItem(raw_data[i]))

            # Falls weniger als 7 Bytes, leere Zellen setzen
            for i in range(length, 7):
                self.can_id_table.setItem(found_row, i + 3, QTableWidgetItem(""))

    def parse_can_data(self, can_id, raw_data, timestamp):
        # Mappings für Status
        mappings = {
            "ignition": {0x00: "Off", 0x01: "On", 0x02: "Start"},
            "turn_signal": {0x00: "Off", 0x01: "Right Turn Signal", 0x09: "Right Turn Signal", 0x02: "Left Turn Signal", 0x0A: "Right Turn Signal", 0x03: "Hazards", 0x11: "Hazards", 0x08: "Off"},
            "door": {0x00: "All Closed", 0x01: "Driver's Door Open", 0x02: "Passenger's Door Open",
                     0x03: "Both Doors Open"},
            "headlamp": {0x00: "Off", 0x02: "daytime light", 0x03: "Low Beam"},
            "gear": {0x35: "5th Gear", 0x34: "4th Gear", 0x33: "3rd Gear", 0x32: "2nd Gear", 0x31: "1st Gear",
                     0x4E: "Neutral", 0x52: "Reverse"}
        }

        def get_state(parameter, value):
            return mappings.get(parameter, {}).get(value, "Unknown")

        if can_id == "423" and len(raw_data) >= 4:
            states = {
                "Ignition": get_state("ignition", int(raw_data[0], 16)),
                "Turn Signals": get_state("turn_signal", int(raw_data[1], 16)),
                "Doors": get_state("door", int(raw_data[2], 16)),
                "Headlamps": get_state("headlamp", int(raw_data[3], 16))
            }
            for param, state in states.items():
                self.update_extra_table(param, state, timestamp)

        elif can_id == "212" and len(raw_data) >= 4:
            rpm = (int(raw_data[2], 16) << 8) | int(raw_data[3], 16)
            self.update_extra_table("RPM", rpm, timestamp)

        elif can_id == "23A" and len(raw_data) >= 2:
            speed = (int(raw_data[0], 16) << 8) | int(raw_data[1], 16)
            self.update_extra_table("Speed", speed, timestamp)

        elif can_id == "418" and len(raw_data) >= 1:
            gear = get_state("gear", int(raw_data[0], 16))
            self.update_extra_table("Gear", gear, timestamp)


    def update_extra_table(self, parameter, value, timestamp):
        # Werte wie Ignition, Turn Signals, Doors, Headlamps aktualisieren
        row_count = self.extra_data_table.rowCount()
        # Überprüfen, ob die Zeile für das Parameter bereits existiert
        parameter_found = False
        for row in range(row_count):
            if self.extra_data_table.item(row, 0).text() == parameter:
                self.extra_data_table.setItem(row, 1, QTableWidgetItem(str(value)))
                parameter_found = True
                break

        if not parameter_found:
            self.extra_data_table.insertRow(row_count)
            self.extra_data_table.setItem(row_count, 0, QTableWidgetItem(parameter))
            self.extra_data_table.setItem(row_count, 1, QTableWidgetItem(str(value)))


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = CANMonitorApp()
    window.show()
    sys.exit(app.exec_())
