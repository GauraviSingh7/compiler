// ============================================================
// server.js — Express backend: compiles Pascal source via C++ compiler
// Returns structured JSON: tokens, symtable, errors, tac, codegen, tree
// ============================================================
import express from "express";
import cors from "cors";
import { execFile } from "child_process";
import { writeFile, unlink, readFile, access, chmod } from "fs/promises";
import { tmpdir } from "os";
import { join } from "path";
import { randomBytes } from "crypto";

const app = express();
app.use(cors());
app.use(express.json({ limit: "2mb" }));

const PORT = process.env.PORT || 3001;
const COMPILER_PATH = join(process.cwd(), "compiler");

// ── POST /compile ─────────────────────────────────────────
app.post("/compile", async (req, res) => {
  const { source } = req.body;
  if (!source || typeof source !== "string") {
    return res.status(400).json({ error: "Missing source field" });
  }

  const id = randomBytes(6).toString("hex");
  const srcFile = join(tmpdir(), `pascal_${id}.pas`);
  const jsonOut = join(tmpdir(), `pascal_${id}.json`);

  try {
    await writeFile(srcFile, source, "utf8");
    await chmod(COMPILER_PATH, 0o755).catch(() => {});

    // Run the compiler — it exits non-zero when source has errors,
    // but it STILL writes the JSON file. We must not treat a non-zero
    // exit as a server error; we treat it as "compilation had errors".
    await new Promise((resolve) => {
      execFile(
        COMPILER_PATH,
        [srcFile, "--json", jsonOut],
        { timeout: 15000 },
        (err, stdout, stderr) => {
          // Always resolve — we'll check for the JSON file next.
          // A non-zero exit just means the Pascal source had errors.
          if (err) {
            console.log(
              `Compiler exited with code ${err.code} — checking for JSON output`
            );
          }
          resolve({ err, stdout, stderr });
        }
      );
    });

    // Check the JSON file actually exists before trying to read it.
    // If it doesn't, that's a true server-side failure (binary crashed,
    // wrong path, permissions, etc.).
    const jsonExists = await access(jsonOut)
      .then(() => true)
      .catch(() => false);

    if (!jsonExists) {
      console.error(
        "Compiler did not produce JSON output. Binary may have crashed."
      );
      return res.status(500).json({
        error:
          "Compiler did not produce output. Check that the binary is a valid Linux x86-64 executable and that COMPILER_PATH is set correctly.",
      });
    }

    const raw = await readFile(jsonOut, "utf8");
    const result = JSON.parse(raw);
    res.json(result);
  } catch (err) {
    console.error("Server error:", err.message);
    res.status(500).json({ error: err.message });
  } finally {
    await unlink(srcFile).catch(() => {});
    await unlink(jsonOut).catch(() => {});
  }
});

// ── Health check ──────────────────────────────────────────
app.get("/health", (_req, res) => res.json({ ok: true }));

app.listen(PORT, () =>
  console.log(`Compiler backend listening on :${PORT}`)
);