# generar_graficas.py
# Genera las graficas del taller (tiempo, speedup, eficiencia y escalabilidad)
# a partir de resultados_resumen.csv.
# Uso: python3 generar_graficas.py <resultados_resumen.csv> [carpeta_salida]
import pandas as pd, matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys, os
if len(sys.argv) < 2:
    print('Uso: python3 generar_graficas.py <resultados_resumen.csv> [carpeta_salida]'); sys.exit(1)
d=pd.read_csv(sys.argv[1])
SAL = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(sys.argv[1])), 'graficas')
os.makedirs(SAL, exist_ok=True)
plt.rcParams.update({'font.family':'DejaVu Serif','font.size':9,'axes.grid':True,'grid.alpha':0.3})
V=[('manual_omp','Paralelo manual','#1f77b4','o'),('ia','Paralelo IA','#d62728','s'),('ia_opt','Paralelo IA + Opt.','#2ca02c','^')]
H=[1,2,4,8,16]; K=[3,5,7,11]; S=[512,1024,2048,4096]
def setx(ax):
    ax.set_xscale('log',base=2); ax.set_xticks(H); ax.set_xticklabels(H); ax.set_xlabel('Número de hilos')
def panel(metric, size, fname, ylabel, title):
    fig,axs=plt.subplots(2,2,figsize=(7.2,5.6))
    for ax,k in zip(axs.flat,K):
        sub=d[(d.tamano==size)&(d.kernel==k)]
        seq=sub[sub.variante=='secuencial'].iloc[0]
        for v,lab,c,m in V:
            s=sub[sub.variante==v].sort_values('hilos')
            if metric=='media_ms':
                ax.errorbar(s.hilos,s.media_ms,yerr=s.desv_ms,label=lab,color=c,marker=m,ms=4,capsize=2,lw=1.2)
            else:
                ax.plot(s.hilos,s[metric],label=lab,color=c,marker=m,ms=4,lw=1.2)
        if metric=='media_ms':
            ax.axhline(seq.media_ms,color='k',ls='--',lw=1,label='Secuencial')
        elif metric=='speedup':
            ax.plot(H,H,color='gray',ls=':',lw=1,label='Ideal (Sp = p)')
            ax.axhline(1,color='k',ls='--',lw=1,label='Secuencial')
        else:
            ax.axhline(1,color='gray',ls=':',lw=1,label='Ideal (Ep = 1)')
        ax.set_title(f'Kernel {k}×{k}'); setx(ax); ax.set_ylabel(ylabel)
    h,l=axs[0,0].get_legend_handles_labels()
    fig.legend(h,l,loc='lower center',ncol=5,frameon=False,fontsize=8)
    fig.suptitle(title,fontsize=10)
    fig.tight_layout(rect=[0,0.05,1,0.97]); fig.savefig(fname,dpi=300); plt.close(fig)
for s in S:
    panel('media_ms',s,os.path.join(SAL,f'tiempo_{s}.png'),'Tiempo de ejecución (ms)',f'Tiempo de ejecución vs. hilos — imagen {s}×{s} px (media ± desv. est., n = 30)')
    panel('speedup',s,os.path.join(SAL,f'speedup_{s}.png'),'Speedup (Sp = T₁/Tp)',f'Speedup vs. hilos — imagen {s}×{s} px')
    panel('eficiencia',s,os.path.join(SAL,f'eficiencia_{s}.png'),'Eficiencia (Ep = Sp/p)',f'Eficiencia vs. hilos — imagen {s}×{s} px')
# speedup at 2 threads vs size
fig,axs=plt.subplots(2,2,figsize=(7.2,5.6))
for ax,k in zip(axs.flat,K):
    for v,lab,c,m in V:
        s=d[(d.variante==v)&(d.kernel==k)&(d.hilos==8)].sort_values('tamano')
        ax.plot(s.tamano,s.speedup,label=lab,color=c,marker=m,ms=4,lw=1.2)
    ax.axhline(8,color='gray',ls=':',lw=1,label='Ideal 8 hilos')
    ax.set_xscale('log',base=2); ax.set_xticks(S); ax.set_xticklabels(S)
    ax.set_xlabel('Tamaño de imagen (px por lado)'); ax.set_ylabel('Speedup con 8 hilos'); ax.set_title(f'Kernel {k}×{k}')
h,l=axs[0,0].get_legend_handles_labels()
fig.legend(h,l,loc='lower center',ncol=4,frameon=False,fontsize=8)
fig.suptitle('Speedup con 8 hilos según tamaño de imagen',fontsize=10)
fig.tight_layout(rect=[0,0.05,1,0.97]); fig.savefig(os.path.join(SAL,'speedup_8hilos_por_tamano.png'),dpi=300); plt.close(fig)
# Escalabilidad relativa al propio tiempo con 1 hilo
for size in S:
    fig,axs=plt.subplots(2,2,figsize=(7.2,5.6))
    for ax,k in zip(axs.flat,K):
        for v,lab,c,m in V:
            s=d[(d.variante==v)&(d.tamano==size)&(d.kernel==k)].sort_values('hilos')
            t1=s[s.hilos==1].media_ms.iloc[0]
            ax.plot(s.hilos,t1/s.media_ms,label=lab,color=c,marker=m,ms=4,lw=1.2)
        ax.plot(H,H,color='gray',ls=':',lw=1,label='Ideal')
        ax.axvline(8,color='k',ls='--',lw=0.8,alpha=0.5)
        ax.set_title(f'Kernel {k}×{k}'); setx(ax); ax.set_ylabel('T(1 hilo) / T(p hilos)')
    h,l=axs[0,0].get_legend_handles_labels()
    fig.legend(h,l,loc='lower center',ncol=4,frameon=False,fontsize=8)
    fig.suptitle(f'Escalabilidad de cada variante respecto a su propia ejecución con 1 hilo — {size}×{size} px',fontsize=9.5)
    fig.tight_layout(rect=[0,0.05,1,0.97]); fig.savefig(os.path.join(SAL,f'escalabilidad_relativa_{size}.png'),dpi=300); plt.close(fig)

print('Graficas guardadas en:', SAL)
