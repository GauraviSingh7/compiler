import styles from "./Tab.module.css";

export default function TACTab({ tac, success }) {
  if (!success)
    return (
      <div className={styles.empty}>
        TAC not generated — fix errors first.
      </div>
    );

  if (!tac?.length)
    return <div className={styles.empty}>No TAC instructions.</div>;

  return (
    <div className={styles.codeBlock}>
      <div className={styles.codeHeader}>Three-Address Code</div>
      {tac.map((line) => (
        <div
          key={line.index}
          className={`${styles.codeLine} ${
            line.isLabel ? styles.labelLine : ""
          }`}
        >
          <span className={styles.lineIdx}>{line.index}</span>
          <span>{line.text}</span>
        </div>
      ))}
    </div>
  );
}