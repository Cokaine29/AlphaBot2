import requests

url = "https://opensky-network.org/api/states/all"

r = requests.get(url, timeout=20)

print("Status:", r.status_code)

if r.status_code == 200:
    data = r.json()
    print("Aircraft count:", len(data["states"]) if data["states"] else 0)
else:
    print(r.text)