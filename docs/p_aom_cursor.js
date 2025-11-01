(() => {
    const el = document.getElementById('cursor');
    const svg = el?.querySelector('svg');
    const g = svg?.querySelector('g.spin');
    if (!el || !svg || !g) return;

    const ns = 'http://www.w3.org/2000/svg';
    const mk = (name) => document.createElementNS(ns, name);
    const tl = mk('path'), tr = mk('path'), bl = mk('path'), br = mk('path');
    [tl, tr, bl, br].forEach(p => {
        p.setAttribute('stroke', 'white');
        p.setAttribute('stroke-width', '3');
        p.setAttribute('stroke-linecap', 'round');
        p.setAttribute('fill', 'none');
        g.appendChild(p);
    });
    const dot = mk('circle'); dot.setAttribute('r', '2.5'); dot.setAttribute('fill', 'white'); g.appendChild(dot);

    // ===== Config =====
    const baseW = 48, baseH = 48; // ขนาดตอนไม่ล็อก
    const posEase = 0.20;        // ลื่นไหลของตำแหน่ง
    const sizeEase = 0.22;        // ลื่นไหลของขนาด
    const arm = 12;               // ความยาวแขน L
    const pad = 4;                // ระยะย่นจากขอบเข้ามา
    const SELECTOR = 'a,button,iframe';

    // ===== State =====
    let mouseX = 0, mouseY = 0;   // ตำแหน่งเมาส์จริง
    let targetX = 0, targetY = 0; // จุดหมายที่เคลื่อน (กลาง element หรือเมาส์)
    let x = 0, y = 0;             // ตำแหน่งจริงของเคอร์เซอร์ปลอม
    let w = baseW, h = baseH;     // ขนาดจริงของ SVG
    let tw = baseW, th = baseH;   // ขนาดเป้าหมาย
    let lockedEl = null;          // element ที่ล็อกอยู่ (ถ้ามี)

    window.addEventListener('mousemove', (e) => {
        mouseX = e.clientX;
        mouseY = e.clientY;
    }, { passive: true });

    function matchInteractive(node) {
        while (node && node !== document.body) {
            if (node.matches?.(SELECTOR)) return node;
            node = node.parentElement;
        }
        return null;
    }

    function updateSVGGeometry(_w, _h) {
        svg.setAttribute('width', _w);
        svg.setAttribute('height', _h);
        svg.setAttribute('viewBox', `0 0 ${_w} ${_h}`);

        dot.setAttribute('cx', String(_w / 2));
        dot.setAttribute('cy', String(_h / 2));

        // TL
        tl.setAttribute('d', `M ${pad} ${pad + arm} V ${pad} M ${pad} ${pad} H ${pad + arm}`);
        // TR
        tr.setAttribute('d', `M ${_w - pad} ${pad + arm} V ${pad} M ${_w - pad} ${pad} H ${_w - pad - arm}`);
        // BL
        bl.setAttribute('d', `M ${pad} ${_h - pad - arm} V ${_h - pad} M ${pad} ${_h - pad} H ${pad + arm}`);
        // BR
        br.setAttribute('d', `M ${_w - pad} ${_h - pad - arm} V ${_h - pad} M ${_w - pad} ${_h - pad} H ${_w - pad - arm}`);
    }

    function loop() {
        const under = document.elementFromPoint(mouseX, mouseY);

        const stillInsideLocked =
            lockedEl && (under === lockedEl || lockedEl.contains(under));

        if (!stillInsideLocked) {
            lockedEl = matchInteractive(under);
        }

        if (lockedEl) {
            const r = lockedEl.getBoundingClientRect();
            targetX = r.left + r.width / 2;
            targetY = r.top + r.height / 2;
            tw = Math.max(32, r.width);
            th = Math.max(32, r.height);
        } else {
            targetX = mouseX;
            targetY = mouseY;
            tw = baseW;
            th = baseH;
        }

        x += (targetX - x) * posEase;
        y += (targetY - y) * posEase;
        w += (tw - w) * sizeEase;
        h += (th - h) * sizeEase;

        updateSVGGeometry(w, h);

        el.style.transform = `translate(${x}px, ${y}px) translate(-50%, -50%)`;

        requestAnimationFrame(loop);
    }
    loop();

    window.addEventListener('mousedown', () => { svg.style.scale = '0.92'; });
    window.addEventListener('mouseup', () => { svg.style.scale = '1'; });
})();
