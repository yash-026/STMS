body {
  font-family: Arial, sans-serif;
  margin: 0;
  padding: 0;
  background-color: #f4f6f9;
}
.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}
.header {
  background-color: #3c8dbc;
  color: white;
  padding: 15px;
  margin-bottom: 20px;
  border-radius: 5px;
  box-shadow: 0 2px 5px rgba(0,0,0,0.1);
}
.dashboard {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 20px;
}
.card {
  background-color: white;
  border-radius: 5px;
  padding: 20px;
  box-shadow: 0 2px 5px rgba(0,0,0,0.1);
}
.card h3 {
  margin-top: 0;
  color: #3c8dbc;
  border-bottom: 1px solid #eee;
  padding-bottom: 10px;
}
.count-display {
  font-size: 32px;
  text-align: center;
  margin: 20px 0;
}
.count-display span {
  font-weight: bold;
  color: #3c8dbc;
}
.status {
  display: inline-block;
  padding: 5px 10px;
  border-radius: 3px;
  font-weight: bold;
}
.status.low {
  background-color: #00a65a;
  color: white;
}
.status.medium {
  background-color: #f39c12;
  color: white;
}
.status.high {
  background-color: #dd4b39;
  color: white;
}
.alert {
  background-color: #dd4b39;
  color: white;
  padding: 10px;
  border-radius: 3px;
  margin-bottom: 10px;
  display: none;
}
.time-controls {
  margin-bottom: 20px;
}
.time-controls button {
  background-color: #3c8dbc;
  color: white;
  border: none;
  padding: 8px 12px;
  border-radius: 3px;
  cursor: pointer;
  margin-right: 5px;
}
.time-controls button:hover {
  background-color: #367fa9;
}
.vehicle-list {
  list-style-type: none;
  padding: 0;
}
.vehicle-list li {
  padding: 10px;
  border-bottom: 1px solid #eee;
}
.vehicle-list li:last-child {
  border-bottom: none;
}
table {
  width: 100%;
  border-collapse: collapse;
}
table th, table td {
  text-align: left;
  padding: 8px;
  border-bottom: 1px solid #ddd;
}
.priority-status {
  font-size: 18px;
  font-weight: bold;
  margin-bottom: 10px;
}
.priority-status.detected {
  color: #2ecc71;
}
.priority-status.not-detected {
  color: #e74c3c;
}