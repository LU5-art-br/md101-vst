(() => {
  "use strict";

  const REPO = "LU5-art-br/md101-vst";
  const FALLBACK_TAG = "v1.1.2";
  const FALLBACK_DEB_VER = "1.1.2";
  const CACHE_KEY = "md101_latest_release_v1_1_2";
  const CACHE_TTL_MS = 5 * 60 * 1000;

  const FALLBACK_URLS = {
    winExe: `https://github.com/${REPO}/releases/download/${FALLBACK_TAG}/Md-101-${FALLBACK_TAG}-Windows-x64-Setup.exe`,
    winZip: `https://github.com/${REPO}/releases/download/${FALLBACK_TAG}/Md-101-${FALLBACK_TAG}-Windows-x64.zip`,
    linuxRun: `https://github.com/${REPO}/releases/download/${FALLBACK_TAG}/Md-101-${FALLBACK_TAG}-Linux-x64-Installer.run`,
    linuxDeb: `https://github.com/${REPO}/releases/download/${FALLBACK_TAG}/md-101_${FALLBACK_DEB_VER}_amd64.deb`,
    linuxTar: `https://github.com/${REPO}/releases/download/${FALLBACK_TAG}/Md-101-${FALLBACK_TAG}-Linux-x64.tar.gz`,
    releaseUrl: `https://github.com/${REPO}/releases/latest`
  };

  const I18N = {
    en: {
      brandSub: "Periodic Table Synth • LU5",
      navDownload: "Download",
      navEngine: "Sound Engine",
      navFamilies: "Chemical Families",
      navDocs: "Manual & Docs",
      heroTitle: "118 Elements. 118 Living Timbres.",
      heroSubtitle:
        "Md-101 is a 16-voice polyphonic VST3 & Standalone synthesizer built in C++17 / JUCE 8. Every chemical element in the Periodic Table computes its timbre in real time from its chemical family, atomic number (Z), atomic mass (A), and physical phase.",
      chip1: "VST3 + Standalone",
      chip2: "16-Voice Polyphonic",
      chip3: "Windows & Linux (x64)",
      chip4: "MIT Open Source",
      heroWinBtn: "Download for Windows",
      heroLinuxBtn: "Download for Linux",
      heroAllBtn: "All Formats (.zip, .deb, .tar.gz)",
      heroSubnotePrefix: "Release",
      heroReleaseNotes: "View Release Notes on GitHub",
      captionLeft: "Preset #101 Md (Mendelevium) — Real-time stereo oscilloscope & physical-to-sonic telemetry",
      captionRight: "Bilingual EN | PT Studio Interface • 61-Key Velocity MIDI Piano",
      dlKicker: "01 / Official Releases",
      dlTitle: "Download Latest Release",
      dlSubtitle:
        "Pre-built 64-bit installers and portable packages directly from GitHub Releases. Includes both the VST3 plugin and Standalone desktop synthesizer.",
      dlDateLoading: "Latest GitHub release",
      dlNotesLink: "Release Notes",
      dlAllReleases: "All Releases",
      recommendedBadge: "Recommended",
      winDesc:
        "Includes the bilingual Dark Studio Setup Wizard (.exe) or a portable archive with install scripts. Zero extra runtime DLLs required.",
      winPrimaryLabel: "Setup Wizard (.exe)",
      winZipLabel: "Portable Archive (.zip)",
      winNote1: "Installs VST3 to <code>C:\\Program Files\\Common Files\\VST3\\Md-101.vst3</code>",
      winNote2: "Installs Standalone app &amp; offline HTML manuals",
      winNote3: "Silent CLI mode supported via <code>/S</code> flag",
      linuxDesc:
        "Universal self-extracting installer (.run) for any distribution, native Debian/Ubuntu package (.deb), or portable tarball (.tar.gz).",
      linuxPrimaryLabel: "Universal Installer (.run)",
      linuxTarLabel: "Portable (.tar.gz)",
      linuxNote1: "User mode (no sudo): installs to <code>~/.vst3/Md-101.vst3</code> &amp; <code>~/.local/bin/Md-101</code>",
      linuxNote2: "System-wide mode supported with <code>--system</code>",
      copyBtn: "Copy",
      copiedBtn: "Copied!",
      engineKicker: "02 / Chemistry → DSP",
      engineTitle: "Physical Properties Computed into Sound",
      engineSubtitle:
        "Rather than static sample banks, Md-101 synthesizes every preset procedurally in real time using analytical DSP modules parameterized by atomic data.",
      feat1Title: "Hybrid Synthesis Architecture",
      feat1Desc:
        "Combines band-limited Virtual Analog oscillators, 2-operator FM bell resonators, sinusoidal wavefolding, and gaseous breath / Geiger scintillation generators.",
      feat2Title: "Zero-Delay Feedback Filter & FX",
      feat2Desc:
        "4-pole Topology-Preserving Transform State-Variable Filter feeding a stereo Chorus and spatial Reverb chain for smooth harmonic sculpting without aliasing.",
      feat3Title: "Atomic Number & Mass Mapping",
      feat3Desc:
        "Atomic number Z drives harmonic density, FM index, and unison detune; atomic mass A governs spectral weight, filter brightness, and envelope inertia.",
      feat4Title: "Built for DAW & Live Play",
      feat4Desc:
        "Automate the <code>element</code> (1–118) parameter in any VST3 DAW, play velocity-sensitive notes across the 61-key MIDI piano, or perform directly on your QWERTY keyboard.",
      famKicker: "03 / Timbral Palette",
      famTitle: "10 Chemical Families",
      famSubtitle:
        "Elements in the same chemical group share a cohesive synthesis architecture. Select any family below to inspect its sonic signature and showcase element.",
      barHarmonics: "Harmonic Density",
      barMass: "Sub-Mass / Inertia",
      barBright: "Spectral Filter",
      barSpace: "Resonance & Tail",
      originKicker: "04 / Story & Documentation",
      originTitle: "Why the name Md-101?",
      storyHeading: "Where Dmitri Mendeleev meets classic synth lore",
      storyP1:
        "In 1869, Russian chemist Dmitri Mendeleev formulated the Periodic Law. Decades later, in 1955, synthetic radioactive element 101 was identified atom by atom at UC Berkeley and named Mendelevium (Md) in his honor.",
      storyP2:
        "Combined with the classic hyphenated naming tradition of iconic studio synthesizers like the Roland SH-101, Yamaha DX-7, and Oberheim OB-X, Md-101 unites periodic chemistry with expressive electronic sound design.",
      docsHeading: "Documentation & Source",
      docManualTitle: "Interactive User Manual",
      docManualSub: "DAW setup, QWERTY shortcuts, MIDI guide & FAQ",
      docArchTitle: "DSP & Software Architecture",
      docArchSub: "Threading model, TPT SVF math & verification suite",
      docSourceTitle: "Source Code on GitHub",
      docSourceSub: "C++17 / JUCE 8 • Build scripts & issue tracker",
      footerTagline: "Periodic Table Polyphonic VST3 Synthesizer",
      footerBy: "Developed by",
      footerLegal:
        "Source code released under the MIT License. Distributed binaries combine JUCE under GPLv3 terms.",
      pillPrefix: "Latest Release",
      pillSuffix: "Windows & Linux",
      publishedPrefix: "Published"
    },
    pt: {
      brandSub: "Sintetizador da Tabela Periódica • LU5",
      navDownload: "Download",
      navEngine: "Motor Sonoro",
      navFamilies: "Famílias Químicas",
      navDocs: "Manual e Docs",
      heroTitle: "118 Elementos. 118 Timbres Vivos.",
      heroSubtitle:
        "Md-101 é um sintetizador polifônico de 16 vozes (VST3 & Standalone) em C++17 / JUCE 8. Cada elemento químico da Tabela Periódica calcula seu timbre em tempo real a partir da família química, número atômico (Z), massa atômica (A) e estado físico.",
      chip1: "VST3 + Standalone",
      chip2: "Polifonia de 16 Vozes",
      chip3: "Windows & Linux (x64)",
      chip4: "Código Aberto MIT",
      heroWinBtn: "Baixar para Windows",
      heroLinuxBtn: "Baixar para Linux",
      heroAllBtn: "Todos os Formatos (.zip, .deb, .tar.gz)",
      heroSubnotePrefix: "Versão",
      heroReleaseNotes: "Ver Notas da Versão no GitHub",
      captionLeft: "Preset #101 Md (Mendelévio) — Osciloscópio estéreo em tempo real e telemetria físico-sonora",
      captionRight: "Interface Bilíngue EN | PT • Piano MIDI de 61 Teclas sensível à velocidade",
      dlKicker: "01 / Lançamentos Oficiais",
      dlTitle: "Baixar Última Versão",
      dlSubtitle:
        "Instaladores 64-bit e pacotes portáteis compilados diretamente via GitHub Releases. Inclui o plugin VST3 e o aplicativo Standalone.",
      dlDateLoading: "Última versão no GitHub",
      dlNotesLink: "Notas da Versão",
      dlAllReleases: "Todas as Versões",
      recommendedBadge: "Recomendado",
      winDesc:
        "Inclui o Assistente de Instalação Dark Studio bilíngue (.exe) ou arquivo portátil com scripts de instalação. Sem dependências extras de DLL.",
      winPrimaryLabel: "Instalador (.exe)",
      winZipLabel: "Arquivo Portátil (.zip)",
      winNote1: "Instala o VST3 em <code>C:\\Program Files\\Common Files\\VST3\\Md-101.vst3</code>",
      winNote2: "Instala o aplicativo Standalone e manuais HTML offline",
      winNote3: "Suporta instalação silenciosa via linha de comando com <code>/S</code>",
      linuxDesc:
        "Instalador universal autoextraível (.run) para qualquer distribuição, pacote nativo Debian/Ubuntu (.deb) ou arquivo portátil (.tar.gz).",
      linuxPrimaryLabel: "Instalador Universal (.run)",
      linuxTarLabel: "Portátil (.tar.gz)",
      linuxNote1: "Modo usuário (sem sudo): instala em <code>~/.vst3/Md-101.vst3</code> e <code>~/.local/bin/Md-101</code>",
      linuxNote2: "Suporta instalação global no sistema com <code>--system</code>",
      copyBtn: "Copiar",
      copiedBtn: "Copiado!",
      engineKicker: "02 / Química → DSP",
      engineTitle: "Propriedades Físicas Calculadas em Som",
      engineSubtitle:
        "Em vez de bancos estáticos de samples, o Md-101 sintetiza cada preset proceduralmente em tempo real usando módulos DSP parametrizados por dados atômicos.",
      feat1Title: "Arquitetura de Síntese Híbrida",
      feat1Desc:
        "Combina osciladores Virtual Analog (PolyBLEP), ressonadores de sino FM de 2 operadores, wavefolding senoidal e geradores de sopro gasoso e cintilação Geiger.",
      feat2Title: "Filtro Zero-Delay Feedback & Efeitos",
      feat2Desc:
        "Filtro TPT State-Variable de 24 dB/oct alimentando uma cadeia estéreo de Chorus e Reverb espacial para modelagem harmônica limpa e sem aliasing.",
      feat3Title: "Mapeamento de Número (Z) e Massa (A)",
      feat3Desc:
        "O número atômico Z controla densidade harmônica, índice FM e desafinação em uníssono; a massa atômica A define peso espectral, brilho do filtro e inércia.",
      feat4Title: "Pronto para DAW & Performance ao Vivo",
      feat4Desc:
        "Automatize o parâmetro <code>element</code> (1–118) em qualquer DAW VST3, toque com sensibilidade de velocidade no piano de 61 teclas ou use o teclado QWERTY.",
      famKicker: "03 / Paleta Tímbrica",
      famTitle: "10 Famílias Químicas",
      famSubtitle:
        "Elementos de um mesmo grupo compartilham uma arquitetura de síntese coesa. Selecione uma família abaixo para explorar sua assinatura sonora e elemento em destaque.",
      barHarmonics: "Densidade Harmônica",
      barMass: "Sub-Massa / Inércia",
      barBright: "Filtro Espectral",
      barSpace: "Ressonância & Cauda",
      originKicker: "04 / História & Documentação",
      originTitle: "Por que o nome Md-101?",
      storyHeading: "Onde Dmitri Mendeleev encontra os sintetizadores clássicos",
      storyP1:
        "Em 1869, o químico russo Dmitri Mendeleev formulou a Lei Periódica. Décadas depois, em 1955, o elemento radioativo sintético 101 foi identificado átomo por átomo em Berkeley e batizado de Mendelévio (Md) em sua homenagem.",
      storyP2:
        "Unido à tradição de siglas alfanuméricas de sintetizadores lendários como Roland SH-101, Yamaha DX-7 e Oberheim OB-X, o Md-101 conecta química periódica e síntese sonora expressiva.",
      docsHeading: "Documentação & Código-Fonte",
      docManualTitle: "Manual Interativo do Usuário",
      docManualSub: "Configuração em DAWs, atalhos QWERTY, guia MIDI e FAQ",
      docArchTitle: "Arquitetura de Software & DSP",
      docArchSub: "Modelo de threads, matemática TPT SVF e suíte de testes",
      docSourceTitle: "Código-Fonte no GitHub",
      docSourceSub: "C++17 / JUCE 8 • Scripts de build e issues",
      footerTagline: "Sintetizador Polifônico VST3 Baseado na Tabela Periódica",
      footerBy: "Desenvolvido por",
      footerLegal:
        "Código-fonte sob licença MIT. Binários distribuídos combinam JUCE sob os termos da GPLv3.",
      pillPrefix: "Última Versão",
      pillSuffix: "Windows & Linux",
      publishedPrefix: "Publicado em"
    }
  };

  const FAMILIES = [
    {
      id: "actinides",
      color: "#DCADD7",
      range: "Ac .. Lr",
      num: "101",
      sym: "Md",
      mass: "[258] u",
      bars: [88, 94, 52, 90],
      en: {
        cat: "Actinides",
        element: "Mendelevium (#101)",
        desc: "Deep radioactive drones with nuclear sub-mass, unstable pitch drift, and Geiger micro-scintillation."
      },
      pt: {
        cat: "Actinídios",
        element: "Mendelévio (#101)",
        desc: "Drones radioativos profundos com sub-massa nuclear, deriva instável de afinação e micro-cintilação Geiger."
      }
    },
    {
      id: "noble",
      color: "#3D9EE5",
      range: "He .. Og",
      num: "2",
      sym: "He",
      mass: "4.003 u",
      bars: [42, 24, 76, 95],
      en: {
        cat: "Noble gases",
        element: "Helium (#2)",
        desc: "Ethereal atmospheric pads with filtered gaseous breath, soft attack transients, and expansive spatial reverb."
      },
      pt: {
        cat: "Gases nobres",
        element: "Hélio (#2)",
        desc: "Pads atmosféricos etéreos com sopro gasoso filtrado, ataque suave e longa reverberação espacial."
      }
    },
    {
      id: "nonmetals",
      color: "#A3D246",
      range: "H, C, N, O, P, S, Se",
      num: "1",
      sym: "H",
      mass: "1.008 u",
      bars: [54, 18, 84, 62],
      en: {
        cat: "Nonmetals",
        element: "Hydrogen (#1)",
        desc: "Organic analog Poly/Wind synthesis; diatomic gases (H, N, O) weave airy breath turbulence into the core."
      },
      pt: {
        cat: "Não metais",
        element: "Hidrogênio (#1)",
        desc: "Síntese analógica orgânica Poly/Wind; gases diatômicos (H, N, O) incorporam turbulência de sopro aéreo."
      }
    },
    {
      id: "halogens",
      color: "#72CBEB",
      range: "F .. Ts",
      num: "17",
      sym: "Cl",
      mass: "35.45 u",
      bars: [78, 44, 92, 68],
      en: {
        cat: "Halogens",
        element: "Chlorine (#17)",
        desc: "Reactive, ionized acid leads and pads driven by high-resonance filters and asymmetric pulse width."
      },
      pt: {
        cat: "Halogênios",
        element: "Cloro (#17)",
        desc: "Leads e pads ácidos reativos e ionizados com filtro de alta ressonância e largura de pulso assimétrica."
      }
    },
    {
      id: "alkali",
      color: "#F3B202",
      range: "Li .. Fr",
      num: "11",
      sym: "Na",
      mass: "22.99 u",
      bars: [64, 38, 86, 40],
      en: {
        cat: "Alkali metals",
        element: "Sodium (#11)",
        desc: "Percussive electric plucks featuring an energetic alkaline pitch spark on the attack transient."
      },
      pt: {
        cat: "Metais alcalinos",
        element: "Sódio (#11)",
        desc: "Plucks elétricos percussivos com centelha rápida de pitch alcalino no transiente de ataque."
      }
    },
    {
      id: "alkaline",
      color: "#EDD805",
      range: "Be .. Ra",
      num: "20",
      sym: "Ca",
      mass: "40.08 u",
      bars: [70, 74, 66, 56],
      en: {
        cat: "Alkaline earth metals",
        element: "Calcium (#20)",
        desc: "Grounded, warm analog Brass-Poly timbres anchored by a dense -1 octave sub-harmonic foundation."
      },
      pt: {
        cat: "Metais alcalino-terrosos",
        element: "Cálcio (#20)",
        desc: "Timbres Brass-Poly analógicos encorpados com sólida fundação sub-harmônica (-1 oitava)."
      }
    },
    {
      id: "transition",
      color: "#EF8E8D",
      range: "Sc .. Cn",
      num: "79",
      sym: "Au",
      mass: "196.97 u",
      bars: [85, 80, 78, 84],
      en: {
        cat: "Transition metals",
        element: "Gold (#79)",
        desc: "Metallic 2-operator FM bell resonators with harmonic ratios shaped by each element's d-block column."
      },
      pt: {
        cat: "Metais de transição",
        element: "Ouro (#79)",
        desc: "Ressonadores e sinos metálicos FM de 2 operadores com razões harmônicas moduladas pela coluna do bloco d."
      }
    },
    {
      id: "semimetals",
      color: "#4DB6AC",
      range: "B, Si, Ge, As, Sb, Te, Po",
      num: "14",
      sym: "Si",
      mass: "28.09 u",
      bars: [76, 42, 88, 60],
      en: {
        cat: "Semimetals",
        element: "Silicon (#14)",
        desc: "Crystalline semiconductor textures crafted with non-linear sinusoidal wavefolding and glassy overtones."
      },
      pt: {
        cat: "Semimetais",
        element: "Silício (#14)",
        desc: "Timbres cristalinos semicondutores com wavefolder senoidal não linear e harmônicos vítreos."
      }
    },
    {
      id: "othermetals",
      color: "#A3C7D7",
      range: "Al, Ga, In, Sn, Tl, Pb, Bi",
      num: "50",
      sym: "Sn",
      mass: "118.71 u",
      bars: [62, 68, 58, 64],
      en: {
        cat: "Other metals",
        element: "Tin (#50)",
        desc: "Malleable post-transition tones blending warm electric tines, tube-style saturation, and ductile vibrato."
      },
      pt: {
        cat: "Outros metais",
        element: "Estanho (#50)",
        desc: "Timbres maleáveis com tines elétricos quentes, saturação valvulada e vibrato dúctil."
      }
    },
    {
      id: "lanthanides",
      color: "#92E3E4",
      range: "La .. Lu",
      num: "60",
      sym: "Nd",
      mass: "144.24 u",
      bars: [92, 70, 82, 88],
      en: {
        cat: "Lanthanides",
        element: "Neodymium (#60)",
        desc: "Luminescent rare-earth ensemble with detuned +1/+2 octave super-harmonics and laser-like shimmer."
      },
      pt: {
        cat: "Lantanídeos",
        element: "Neodímio (#60)",
        desc: "Ensemble luminescente de terras-raras com super-harmônicos desafinados (+1/+2 oitavas) e brilho a laser."
      }
    }
  ];

  let currentLang = detectInitialLang();
  let activeFamilyIndex = 0;
  let releaseState = {
    tag: FALLBACK_TAG,
    publishedAt: null
  };

  function detectInitialLang() {
    try {
      const saved = localStorage.getItem("md101_lang");
      if (saved === "en" || saved === "pt") return saved;
    } catch (_) {}
    const navLang = (navigator.language || "en").toLowerCase();
    return navLang.startsWith("pt") ? "pt" : "en";
  }

  function detectOS() {
    const platform = (
      (navigator.userAgentData && navigator.userAgentData.platform) ||
      navigator.platform ||
      navigator.userAgent ||
      ""
    ).toLowerCase();
    if (platform.includes("win")) return "windows";
    if (platform.includes("linux") || platform.includes("x11")) return "linux";
    return "other";
  }

  function applyOSHighlight() {
    const os = detectOS();
    const winCard = document.getElementById("card-windows");
    const linuxCard = document.getElementById("card-linux");
    const heroWin = document.getElementById("hero-win-btn");
    const heroLinux = document.getElementById("hero-linux-btn");

    if (os === "windows") {
      winCard?.classList.add("recommended");
      heroWin?.classList.add("btn-highlight");
    } else if (os === "linux") {
      linuxCard?.classList.add("recommended");
      heroLinux?.classList.add("btn-highlight");
    }
  }

  function formatBytes(bytes) {
    if (!Number.isFinite(bytes) || bytes <= 0) return "";
    const mb = bytes / (1024 * 1024);
    return `${mb.toFixed(1)} MB`;
  }

  function formatDate(isoString, lang) {
    if (!isoString) return I18N[lang].dlDateLoading;
    const d = new Date(isoString);
    if (Number.isNaN(d.getTime())) return I18N[lang].dlDateLoading;
    const formatted = d.toLocaleDateString(lang === "pt" ? "pt-BR" : "en-US", {
      year: "numeric",
      month: "short",
      day: "numeric"
    });
    return `${I18N[lang].publishedPrefix} ${formatted}`;
  }

  function applyLanguage(lang) {
    currentLang = lang;
    document.documentElement.lang = lang === "pt" ? "pt-BR" : "en";

    const dict = I18N[lang];
    document.querySelectorAll("[data-i18n]").forEach((el) => {
      const key = el.getAttribute("data-i18n");
      if (dict[key] !== undefined) {
        el.innerHTML = dict[key];
      }
    });

    const enSpan = document.getElementById("lang-en");
    const ptSpan = document.getElementById("lang-pt");
    enSpan?.classList.toggle("active-lang", lang === "en");
    ptSpan?.classList.toggle("active-lang", lang === "pt");

    const manualLink = document.getElementById("manual-link");
    const archLink = document.getElementById("arch-link");
    if (manualLink) {
      manualLink.href = lang === "pt" ? "docs/manual.html" : "docs/manual-en.html";
    }
    if (archLink) {
      archLink.href =
        lang === "pt"
          ? `https://github.com/${REPO}/blob/main/docs/ARCHITECTURE.md`
          : `https://github.com/${REPO}/blob/main/docs/ARCHITECTURE-en.md`;
    }

    updateReleaseBannerText();
    renderFamilies();
    selectFamily(activeFamilyIndex);

    try {
      localStorage.setItem("md101_lang", lang);
    } catch (_) {}
  }

  function updateReleaseBannerText() {
    const dict = I18N[currentLang];
    const pillText = document.getElementById("release-pill-text");
    if (pillText) {
      pillText.textContent = `${dict.pillPrefix} • ${releaseState.tag} • ${dict.pillSuffix}`;
    }
    const dlDate = document.getElementById("dl-date");
    if (dlDate) {
      dlDate.textContent = formatDate(releaseState.publishedAt, currentLang);
    }
  }

  function renderFamilies() {
    const grid = document.getElementById("families-grid");
    if (!grid) return;

    grid.innerHTML = "";
    FAMILIES.forEach((fam, idx) => {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.className = "family-chip" + (idx === activeFamilyIndex ? " active" : "");
      btn.style.setProperty("--fam-color", fam.color);
      btn.setAttribute("role", "tab");
      btn.setAttribute("aria-selected", idx === activeFamilyIndex ? "true" : "false");

      const localized = fam[currentLang];
      btn.innerHTML = `
        <span class="fam-swatch" aria-hidden="true"></span>
        <span>
          <span class="fam-chip-title">${localized.cat}</span>
          <span class="fam-chip-range">${fam.range}</span>
        </span>
      `;
      btn.addEventListener("click", () => selectFamily(idx));
      grid.appendChild(btn);
    });
  }

  function selectFamily(index) {
    activeFamilyIndex = index;
    const fam = FAMILIES[index];
    if (!fam) return;

    const localized = fam[currentLang];
    const inspector = document.getElementById("family-inspector");
    if (inspector) {
      inspector.style.setProperty("--fam-color", fam.color);
    }

    const setText = (id, value) => {
      const el = document.getElementById(id);
      if (el) el.textContent = value;
    };

    setText("insp-num", fam.num);
    setText("insp-sym", fam.sym);
    setText("insp-mass", fam.mass);
    setText("insp-cat", localized.cat);
    setText("insp-name", localized.element);
    setText("insp-desc", localized.desc);

    const barIds = ["bar-harmonics", "bar-mass", "bar-bright", "bar-space"];
    barIds.forEach((id, i) => {
      const bar = document.getElementById(id);
      if (bar) bar.style.width = `${fam.bars[i]}%`;
    });

    document.querySelectorAll(".family-chip").forEach((chip, i) => {
      const isSelected = i === index;
      chip.classList.toggle("active", isSelected);
      chip.setAttribute("aria-selected", isSelected ? "true" : "false");
    });
  }

  function applyReleaseData(release) {
    if (!release || !release.tag_name) return;

    const tag = release.tag_name;
    releaseState.tag = tag;
    releaseState.publishedAt = release.published_at || null;

    const assets = Array.isArray(release.assets) ? release.assets : [];
    const findAsset = (regex) => assets.find((a) => regex.test(a.name || ""));

    const winExe = findAsset(/windows.*\.exe$/i);
    const winZip = findAsset(/windows.*\.zip$/i);
    const linuxRun = findAsset(/linux.*\.run$/i);
    const linuxDeb = findAsset(/\.deb$/i);
    const linuxTar = findAsset(/linux.*\.tar\.gz$/i);
    const checksums = findAsset(/sha256sums/i);

    const releaseUrl = release.html_url || FALLBACK_URLS.releaseUrl;

    const heroVersion = document.getElementById("hero-version-inline");
    if (heroVersion) heroVersion.textContent = tag;
    const dlVersion = document.getElementById("dl-version");
    if (dlVersion) dlVersion.textContent = tag;

    const setLink = (id, url) => {
      const el = document.getElementById(id);
      if (el && url) el.href = url;
    };

    setLink("hero-notes-link", releaseUrl);
    setLink("dl-notes-link", releaseUrl);
    setLink("dl-checksums-link", checksums ? checksums.browser_download_url : releaseUrl);

    if (winExe) {
      setLink("hero-win-btn", winExe.browser_download_url);
      setLink("win-exe-btn", winExe.browser_download_url);
      const meta = document.getElementById("win-exe-meta");
      if (meta) {
        const size = formatBytes(winExe.size);
        meta.textContent = size ? `x64 • ${size}` : "Windows x64";
      }
    }

    if (winZip) {
      setLink("win-zip-link", winZip.browser_download_url);
      const sizeEl = document.getElementById("win-zip-size");
      if (sizeEl && winZip.size) sizeEl.textContent = `(${formatBytes(winZip.size)})`;
    }

    if (linuxRun) {
      setLink("hero-linux-btn", linuxRun.browser_download_url);
      setLink("linux-run-btn", linuxRun.browser_download_url);
      const meta = document.getElementById("linux-run-meta");
      if (meta) {
        const size = formatBytes(linuxRun.size);
        meta.textContent = size ? `x64 • ${size}` : "Linux x64";
      }
      const cmdEl = document.getElementById("linux-cmd");
      if (cmdEl && linuxRun.name) {
        cmdEl.textContent = `chmod +x ${linuxRun.name}\n./${linuxRun.name}`;
      }
    }

    if (linuxDeb) {
      setLink("linux-deb-link", linuxDeb.browser_download_url);
      const sizeEl = document.getElementById("linux-deb-size");
      if (sizeEl && linuxDeb.size) sizeEl.textContent = `(${formatBytes(linuxDeb.size)})`;
    }

    if (linuxTar) {
      setLink("linux-tar-link", linuxTar.browser_download_url);
      const sizeEl = document.getElementById("linux-tar-size");
      if (sizeEl && linuxTar.size) sizeEl.textContent = `(${formatBytes(linuxTar.size)})`;
    }

    updateReleaseBannerText();
  }

  async function loadLatestRelease() {
    try {
      const cachedRaw = sessionStorage.getItem(CACHE_KEY);
      if (cachedRaw) {
        const cached = JSON.parse(cachedRaw);
        if (cached && Date.now() - cached.ts < CACHE_TTL_MS && cached.data) {
          applyReleaseData(cached.data);
          return;
        }
      }
    } catch (_) {}

    try {
      const resp = await fetch(`https://api.github.com/repos/${REPO}/releases/latest`, {
        headers: { Accept: "application/vnd.github+json" }
      });
      if (!resp.ok) return;
      const data = await resp.json();
      applyReleaseData(data);
      try {
        sessionStorage.setItem(CACHE_KEY, JSON.stringify({ ts: Date.now(), data }));
      } catch (_) {}
    } catch (_) {
      // Fallback static v1.1.2 links remain active if offline or rate-limited
    }
  }

  function setupInteractions() {
    const langBtn = document.getElementById("lang-toggle");
    langBtn?.addEventListener("click", () => {
      applyLanguage(currentLang === "en" ? "pt" : "en");
    });

    const copyBtn = document.getElementById("copy-linux-cmd");
    const cmdPre = document.getElementById("linux-cmd");
    copyBtn?.addEventListener("click", async () => {
      if (!cmdPre) return;
      try {
        await navigator.clipboard.writeText(cmdPre.textContent.trim());
        copyBtn.textContent = I18N[currentLang].copiedBtn;
        setTimeout(() => {
          copyBtn.textContent = I18N[currentLang].copyBtn;
        }, 1600);
      } catch (_) {}
    });
  }

  document.addEventListener("DOMContentLoaded", () => {
    applyOSHighlight();
    setupInteractions();
    applyLanguage(currentLang);
    loadLatestRelease();
  });
})();
