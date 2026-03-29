import { useEffect, useRef } from "react";
import styles from "./Tab.module.css";

const NW = 140, NH = 36, HGAP = 16, VGAP = 52;
const COLORS = { rule: "#606060", token: "#0891b2", error: "#dc2626" };
const BGFILL = { rule: "#f9fafb", token: "#f0f9ff", error: "#fef2f2" };

// Deep-clone tree and attach render state
function cloneTree(node) {
  return {
    ...node,
    _col: false,
    _hid: false,
    children: (node.children || []).map(cloneTree),
  };
}

function layout(node, depth, ctr) {
  node._y = depth * (NH + VGAP);
  const vis = node._col
    ? []
    : (node.children || []).filter((c) => !c._hid);

  if (!vis.length) {
    node._x = ctr.v++ * (NW + HGAP);
    return;
  }
  vis.forEach((c) => layout(c, depth + 1, ctr));
  node._x = (vis[0]._x + vis[vis.length - 1]._x) / 2;
}

function maxDepth(n) {
  if (n._col || !n.children?.length) return 0;
  const vis = n.children.filter((c) => !c._hid);
  return vis.length ? 1 + Math.max(...vis.map(maxDepth)) : 0;
}

function setHidden(children, val) {
  children.forEach((c) => {
    c._hid = val;
    setHidden(c.children || [], val);
  });
}

export default function ParseTreeTab({ tree }) {
  const svgRef = useRef(null);
  const rootRef = useRef(null);

  useEffect(() => {
    if (!tree) return;
    rootRef.current = cloneTree(tree);
    render();
  }, [tree]); // eslint-disable-line

  function ns(tag) {
    return document.createElementNS("http://www.w3.org/2000/svg", tag);
  }

  function render() {
    const root = rootRef.current;
    if (!root || !svgRef.current) return;

    const ctr = { v: 0 };
    layout(root, 0, ctr);
    const W = Math.max(ctr.v * (NW + HGAP) + 40, 500);
    const H = (maxDepth(root) + 1) * (NH + VGAP) + 60;

    const svg = svgRef.current;
    svg.setAttribute("width", W);
    svg.setAttribute("height", H);
    svg.innerHTML = "";

    drawEdges(svg, root);
    drawNodes(svg, root);
  }

  function drawEdges(svg, node) {
    if (node._col || !node.children) return;
    node.children
      .filter((c) => !c._hid)
      .forEach((c) => {
        const line = ns("line");
        line.setAttribute("x1", node._x + NW / 2);
        line.setAttribute("y1", node._y + NH);
        line.setAttribute("x2", c._x + NW / 2);
        line.setAttribute("y2", c._y);
        line.setAttribute("stroke", "#d1d5db");
        line.setAttribute("stroke-width", "1.5");
        svg.appendChild(line);
        drawEdges(svg, c);
      });
  }

  function drawNodes(svg, node) {
    if (node._hid) return;

    const g = ns("g");
    g.style.cursor = node.children?.length ? "pointer" : "default";
    g.onclick = (e) => {
      e.stopPropagation();
      toggleCollapse(node);
    };

    // Box
    const rect = ns("rect");
    rect.setAttribute("x", node._x);
    rect.setAttribute("y", node._y);
    rect.setAttribute("width", NW);
    rect.setAttribute("height", NH);
    rect.setAttribute("rx", "6");
    rect.setAttribute("fill", BGFILL[node.kind] || "#f9fafb");
    rect.setAttribute("stroke", COLORS[node.kind] || "#606060");
    rect.setAttribute("stroke-width", node._col ? "2.5" : "1.5");
    g.appendChild(rect);

    // Collapse indicator dot
    if (node.children?.length) {
      const dot = ns("circle");
      dot.setAttribute("cx", node._x + NW - 9);
      dot.setAttribute("cy", node._y + 9);
      dot.setAttribute("r", "3.5");
      dot.setAttribute(
        "fill",
        node._col ? COLORS[node.kind] : "transparent"
      );
      dot.setAttribute("stroke", COLORS[node.kind] || "#606060");
      dot.setAttribute("stroke-width", "1.5");
      g.appendChild(dot);
    }

    // Label — split on \n for two-line display
    const parts = node.label.split("\n");
    const main = parts[0];
    const sub = parts[1] || "";
    const fs = main.length > 16 ? 9 : main.length > 11 ? 10 : 11;
    const yMain = node._y + (sub ? 14 : 22);

    const txt = ns("text");
    txt.setAttribute("x", node._x + NW / 2);
    txt.setAttribute("y", yMain);
    txt.setAttribute("text-anchor", "middle");
    txt.setAttribute("font-size", fs);
    txt.setAttribute("font-family", "JetBrains Mono, Consolas, monospace");
    txt.setAttribute("fill", COLORS[node.kind] || "#606060");
    txt.textContent = main;
    g.appendChild(txt);

    if (sub) {
      const s2 = ns("text");
      s2.setAttribute("x", node._x + NW / 2);
      s2.setAttribute("y", node._y + 28);
      s2.setAttribute("text-anchor", "middle");
      s2.setAttribute("font-size", "8");
      s2.setAttribute("font-family", "JetBrains Mono, Consolas, monospace");
      s2.setAttribute("fill", "#9ca3af");
      s2.textContent = sub;
      g.appendChild(s2);
    }

    svg.appendChild(g);

    if (!node._col) {
      (node.children || [])
        .filter((c) => !c._hid)
        .forEach((c) => drawNodes(svg, c));
    }
  }

  function toggleCollapse(node) {
    if (!node.children?.length) return;
    node._col = !node._col;
    setHidden(node.children, node._col);
    render();
  }

  if (!tree)
    return (
      <div className={styles.empty}>
        No parse tree — fix errors first.
      </div>
    );

  return (
    <div className={styles.treeWrap}>
      <div className={styles.treeLegend}>
        <span>
          <span
            className={styles.dot}
            style={{ background: "#f9fafb", border: "1.5px solid #606060" }}
          />
          Rule
        </span>
        <span>
          <span
            className={styles.dot}
            style={{ background: "#f0f9ff", border: "1.5px solid #0891b2" }}
          />
          Token
        </span>
        <span>
          <span
            className={styles.dot}
            style={{ background: "#fef2f2", border: "1.5px solid #dc2626" }}
          />
          Error
        </span>
        <span className={styles.hint}>Click node to collapse / expand</span>
      </div>
      <div className={styles.treeScroll}>
        <svg ref={svgRef} style={{ display: "block" }} />
      </div>
    </div>
  );
}