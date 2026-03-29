import styles from "./Tab.module.css";

export default function TargetTab({ code, success }) {
  if (!success)
    return (
      <div className={styles.empty}>
        Target code not generated — fix errors first.
      </div>
    );

  if (!code?.length)
    return <div className={styles.empty}>No target instructions.</div>;

  return (
    <div className={styles.codeBlock}>
      <div className={styles.codeHeader}>Pseudo-Assembly Target Code</div>
      {code.map((line) => (
        <div
          key={line.index}
          className={`${styles.codeLine} ${
            line.isLabel ? styles.labelLine : ""
          } ${line.isComment ? styles.commentLine : ""}`}
        >
          <span className={styles.lineIdx}>{line.index}</span>
          <span>{line.text}</span>
        </div>
      ))}
    </div>
  );
}