import { useRef, useCallback } from "react";
import styles from "./Editor.module.css";

// ── Very lightweight Pascal syntax highlighter ────────────
const KEYWORDS = new Set([
  "program","var","begin","end","if","then","else","while","do",
  "read","write","procedure","function","array","of","and","or",
  "not","integer",
]);

function highlight(line) {
  // We process char by char to avoid regex catastrophic backtracking
  const tokens = [];
  let i = 0;
  const push = (cls, text) => tokens.push({ cls, text });

  while (i < line.length) {
    // Comment
    if (line[i] === "!") {
      push("comment", line.slice(i));
      break;
    }
    // String
    if (line[i] === "'") {
      let j = i + 1;
      while (j < line.length && line[j] !== "'") {
        if (line[j] === "\\") j++;
        j++;
      }
      push("string", line.slice(i, j + 1));
      i = j + 1;
      continue;
    }
    // Number
    if (/\d/.test(line[i])) {
      let j = i;
      while (j < line.length && /\d/.test(line[j])) j++;
      push("number", line.slice(i, j));
      i = j;
      continue;
    }
    // Identifier / keyword
    if (/[a-zA-Z_]/.test(line[i])) {
      let j = i;
      while (j < line.length && /[a-zA-Z0-9_]/.test(line[j])) j++;
      const word = line.slice(i, j);
      push(KEYWORDS.has(word.toLowerCase()) ? "keyword" : "ident", word);
      i = j;
      continue;
    }
    // Operators / punctuation
    if (/[+\-*/=<>:;.,()[\]{}]/.test(line[i])) {
      // Grab compound tokens
      const two = line.slice(i, i + 2);
      if ([":=", "<>", "<=", ">="].includes(two)) {
        push("op", two);
        i += 2;
      } else {
        push("op", line[i]);
        i++;
      }
      continue;
    }
    push("plain", line[i]);
    i++;
  }
  return tokens;
}

function HighlightedLine({ text }) {
  const tokens = highlight(text);
  return (
    <span>
      {tokens.map((t, i) => (
        <span key={i} className={styles[t.cls]}>
          {t.text}
        </span>
      ))}
    </span>
  );
}

// ── Tab key support ───────────────────────────────────────
function handleTab(e, value, onChange) {
  if (e.key !== "Tab") return;
  e.preventDefault();
  const start = e.target.selectionStart;
  const end   = e.target.selectionEnd;
  const next  = value.slice(0, start) + "  " + value.slice(end);
  onChange(next);
  requestAnimationFrame(() => {
    e.target.selectionStart = e.target.selectionEnd = start + 2;
  });
}

export default function Editor({ value, onChange }) {
  const taRef = useRef(null);
  const lines  = value.split("\n");

  const onKeyDown = useCallback(
    (e) => handleTab(e, value, onChange),
    [value, onChange]
  );

  return (
    <div className={styles.editorWrap}>
      {/* Gutter */}
      <div className={styles.gutter} aria-hidden="true">
        {lines.map((_, i) => (
          <div key={i} className={styles.lineNum}>{i + 1}</div>
        ))}
      </div>

      {/* Highlight layer */}
      <div className={styles.highlightLayer} aria-hidden="true">
        {lines.map((line, i) => (
          <div key={i} className={styles.hlLine}>
            <HighlightedLine text={line} />
            {/* invisible trailing char keeps height correct */}
            {"\u200b"}
          </div>
        ))}
      </div>

      {/* Actual textarea — transparent over the highlight layer */}
      <textarea
        ref={taRef}
        className={styles.textarea}
        value={value}
        onChange={(e) => onChange(e.target.value)}
        onKeyDown={onKeyDown}
        spellCheck={false}
        autoComplete="off"
        autoCorrect="off"
        autoCapitalize="off"
      />
    </div>
  );
}