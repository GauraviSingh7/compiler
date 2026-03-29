import styles from "./StatusBar.module.css";
export default function StatusBar({ lines, loading, success, errorCount }) {
  return (
    <div className={styles.bar}>
      <span>{lines} lines</span>
      {loading && <span className={styles.compiling}>Compiling…</span>}
      {!loading && success === true && <span className={styles.ok}>✓ Compiled</span>}
      {!loading && success === false && <span className={styles.err}>✗ {errorCount} error{errorCount !== 1 ? "s" : ""}</span>}
      <span className={styles.right}>Pascal · UTF-8</span>
    </div>
  );
}