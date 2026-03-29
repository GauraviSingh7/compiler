// ============================================================
// server.js — Express backend: compiles Pascal source via C++ compiler
// Returns structured JSON: tokens, symtable, errors, tac, codegen, tree
// ============================================================
import express from "express";
import cors from "cors";
import { execFile } from "child_process";
import { writeFile, unlink, readFile } from "fs/promises";
import { tmpdir } from "os";
import { join } from "path";
import { randomBytes } from "crypto";
import { chmod } from "fs/promises";

const app = express();
app.use(cors());
app.use(express.json({ limit: "2mb" }));

const PORT = process.env.PORT || 3001;
const COMPILER_PATH = join(process.cwd(), "compiler");

// ── POST /compile ─────────────────────────────────────────
app.post("/compile", async (req, res) => {
  console.log("REQUEST RECEIVED");       
  console.log("BODY:", req.body);               

  const { source } = req.body;
  console.log("SOURCE:", source);  
  if (!source || typeof source !== "string") {
    return res.status(400).json({ error: "Missing source field" });
  }

  const id = randomBytes(6).toString("hex");
  const srcFile = join(tmpdir(), `pascal_${id}.pas`);
  const jsonOut = join(tmpdir(), `pascal_${id}.json`);

  try {
    await writeFile(srcFile, source, "utf8");

    await chmod(COMPILER_PATH, 0o755).catch(() => {});

    console.log("Running compiler:", COMPILER_PATH);
    console.log("Temp files:", srcFile, jsonOut);

    await new Promise((resolve, reject) => {
      execFile(
        COMPILER_PATH,
        [srcFile, "--json", jsonOut],
        { timeout: 15000 },
        (err, stdout, stderr) => {
          if (err && err.code !== 0 && !stderr.includes("Errors found")) {
            reject(new Error(stderr || err.message));
          } else {
            resolve({ stdout, stderr });
          }
        }
      );
    });

    const raw = await readFile(jsonOut, "utf8");
    const result = JSON.parse(raw);
    res.json(result);
  } catch (err) {
  console.error("ERROR:", err);
  res.status(500).json({ error: err.message });
  }finally {
    await unlink(srcFile).catch(() => {});
    await unlink(jsonOut).catch(() => {});
  }
});

// ── Health check ──────────────────────────────────────────
app.get("/health", (_req, res) => res.json({ ok: true }));

app.listen(PORT, () => console.log(`Compiler backend listening on :${PORT}`));