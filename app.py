from flask import Flask, render_template, request, send_file
import os
import subprocess
import uuid
import time

app = Flask(__name__)

# ---------------- PATHS ----------------
BASE_DIR = os.getcwd()

UPLOAD = os.path.join(BASE_DIR, "uploads")
COMPRESSED = os.path.join(BASE_DIR, "compressed")
DECOMPRESSED = os.path.join(BASE_DIR, "decompressed")

COMPRESS_EXE = os.path.join(BASE_DIR, "huffman_compress.exe")
DECOMPRESS_EXE = os.path.join(BASE_DIR, "huffman_decompress.exe")

for d in (UPLOAD, COMPRESSED, DECOMPRESSED):
    os.makedirs(d, exist_ok=True)

# ---------------- STORE ----------------
file_store = {}
KEY_EXPIRY_SECONDS = 300  # 5 minutes

# ---------------- HOME ----------------
@app.route("/")
def home():
    return render_template("home.html")

# ---------------- SEND ----------------
@app.route("/send", methods=["GET", "POST"])
def send():
    file_type = None
    warning = None
    if request.method == "POST":
        file = request.files.get("file")
        if not file or file.filename == "":
            return render_template("send.html", error="No file selected", file_type=file_type, warning=warning)

        name, ext = os.path.splitext(file.filename)
        uid = str(uuid.uuid4())
        key = str(uuid.uuid4())[:8]

        input_path = os.path.join(UPLOAD, f"{uid}_{file.filename}")
        compressed_path = os.path.join(
            COMPRESSED, f"{uid}_{name}_compressed.huff"
        )

        file.save(input_path)

        # ---- FILE TYPE DETECTION ----
        compressed_exts = {
            ".jpg", ".jpeg", ".png", ".mp3", ".mp4",
            ".zip", ".rar", ".7z", ".pdf", ".exe"
        }

        ext_lower = ext.lower()
        if ext_lower in compressed_exts:
            file_type = "Already compressed file"
            warning = (
                "Warning: This file type is typically already compressed. "
                "Size reduction may be minimal or the compressed file may "
                "even be larger than the original."
            )
        else:
            file_type = "Text / Raw file"
            warning = None

        subprocess.run([COMPRESS_EXE, input_path, compressed_path], check=True)

        # ---- STATS ----
        original_size = os.path.getsize(input_path)
        compressed_size = os.path.getsize(compressed_path)
        ratio = round(original_size / compressed_size, 2) if compressed_size else 0

        file_store[key] = {
            "compressed": compressed_path,
            "name": name,
            "ext": ext,
            "created_at": time.time(),
            "stats": {
                "original": original_size,
                "compressed": compressed_size,
                "ratio": ratio
            }
        }

        return render_template(
            "send.html",
            key=key,
            stats=file_store[key]["stats"],
            file_type=file_type,
            warning=warning
        )

    return render_template("send.html", file_type=file_type, warning=warning)

# ---------------- RECEIVE ----------------
@app.route("/receive", methods=["GET", "POST"])
def receive():
    if request.method == "POST":
        key = request.form.get("key")

        if not key or key not in file_store:
            return render_template("receive.html", error="Invalid key")

        data = file_store[key]

        # ---- EXPIRY CHECK ----
        if time.time() - data["created_at"] > KEY_EXPIRY_SECONDS:
            del file_store[key]
            return render_template(
                "receive.html",
                error="Key expired. Ask sender to generate a new key."
            )

        uid = str(uuid.uuid4())
        original_filename = data["name"] + data["ext"]
        output_path = os.path.join(
            DECOMPRESSED, f"{uid}_{original_filename}"
        )

        subprocess.run(
            [DECOMPRESS_EXE, data["compressed"], output_path],
            check=True
        )

        # one-time key
        del file_store[key]

        return send_file(
            output_path,
            as_attachment=True,
            download_name=original_filename
        )

    return render_template("receive.html")

# ---------------- LOCAL COMPRESS ----------------
@app.route("/compress", methods=["GET", "POST"])
def compress():
    if request.method == "POST":
        file = request.files.get("file")
        if not file or file.filename == "":
            return render_template("compress.html", error="No file selected")

        name, _ = os.path.splitext(file.filename)
        uid = str(uuid.uuid4())

        input_path = os.path.join(UPLOAD, f"{uid}_{file.filename}")
        output_path = os.path.join(COMPRESSED, f"{name}_compressed.huff")

        file.save(input_path)
        subprocess.run([COMPRESS_EXE, input_path, output_path], check=True)

        return send_file(output_path, as_attachment=True)

    return render_template("compress.html")

# ---------------- LOCAL DECOMPRESS ----------------
@app.route("/decompress", methods=["GET", "POST"])
def decompress():
    if request.method == "POST":
        file = request.files.get("file")
        if not file or file.filename == "":
            return render_template("decompress.html", error="No file selected")

        original_name = file.filename.replace("_compressed.huff", "")
        uid = str(uuid.uuid4())

        input_path = os.path.join(UPLOAD, f"{uid}_{file.filename}")
        output_path = os.path.join(DECOMPRESSED, original_name)

        file.save(input_path)
        subprocess.run([DECOMPRESS_EXE, input_path, output_path], check=True)

        return send_file(
            output_path,
            as_attachment=True,
            download_name=original_name
        )

    return render_template("decompress.html")

# ---------------- RUN ----------------
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
